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

import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.search.now.ui.piet.GradientsProto.Fill;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.RoundedCornersProto.RoundedCorners;
import com.google.search.now.ui.piet.StylesProto.EdgeWidths;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.Style;
import java.util.ArrayList;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** TODO: Test remaining methods of StyleProvider. */
@RunWith(RobolectricTestRunner.class)
public class StyleProviderTest {

  @Mock private AssetProvider mockAssetProvider;
  @Mock private CustomElementProvider mockCustomElementProvider;
  @Mock private ActionHandler mockActionHandler;

  private FrameContext frameContext;

  private Context context;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    frameContext =
        FrameContext.createFrameContext(
            Frame.getDefaultInstance(),
            DEFAULT_STYLE,
            new ArrayList<>(),
            DebugBehavior.VERBOSE,
            new DebugLogger(),
            mockAssetProvider,
            mockCustomElementProvider,
            new HostBindingProvider(),
            mockActionHandler);
  }

  @Test
  public void testGetters_withStyleDefined() {
    Style style =
        Style.newBuilder()
            .setColor(1)
            .setBackground(Fill.newBuilder().setColor(2))
            .setRoundedCorners(RoundedCorners.newBuilder().setRadius(3))
            .setFont(Font.newBuilder().setSize(4))
            .setMaxLines(5)
            .setMinHeight(6)
            .setHeight(7)
            .setWidth(8)
            .build();

    StyleProvider styleProvider = new StyleProvider(style);

    assertThat(styleProvider.getColor()).isEqualTo(style.getColor());
    assertThat(styleProvider.getBackground()).isEqualTo(style.getBackground());
    assertThat(styleProvider.getRoundedCorners()).isEqualTo(style.getRoundedCorners());
    assertThat(styleProvider.hasRoundedCorners()).isTrue();
    assertThat(styleProvider.getFont()).isEqualTo(style.getFont());
    assertThat(styleProvider.getMaxLines()).isEqualTo(style.getMaxLines());
    assertThat(styleProvider.getHeight()).isEqualTo(style.getHeight());
    assertThat(styleProvider.getWidth()).isEqualTo(style.getWidth());
    assertThat(styleProvider.hasHeight()).isTrue();
    assertThat(styleProvider.hasWidth()).isTrue();
  }

  @Test
  public void testGetters_withEmptyStyleDefined() {
    Style style = Style.getDefaultInstance();

    StyleProvider styleProvider = new StyleProvider(style);

    assertThat(styleProvider.getColor()).isEqualTo(style.getColor());
    assertThat(styleProvider.getBackground()).isEqualTo(style.getBackground());
    assertThat(styleProvider.getRoundedCorners()).isEqualTo(style.getRoundedCorners());
    assertThat(styleProvider.hasRoundedCorners()).isFalse();
    assertThat(styleProvider.getFont()).isEqualTo(Font.getDefaultInstance());
    assertThat(styleProvider.getMaxLines()).isEqualTo(style.getMaxLines());
    assertThat(styleProvider.getHeight()).isEqualTo(style.getHeight());
    assertThat(styleProvider.getWidth()).isEqualTo(style.getWidth());
    assertThat(styleProvider.hasHeight()).isFalse();
    assertThat(styleProvider.hasWidth()).isFalse();
  }

  @Test
  public void testSetPadding() {
    EdgeWidths padding =
        EdgeWidths.newBuilder().setTop(1).setBottom(2).setStart(3).setEnd(4).build();
    View view = new View(context);
    new StyleProvider(Style.getDefaultInstance()).setPadding(context, view, padding);
    verifyPadding(view, padding);
  }

  @Test
  public void testSetMargins() {
    EdgeWidths margins =
        EdgeWidths.newBuilder().setTop(1).setBottom(2).setStart(3).setEnd(4).build();
    Style marginStyle = Style.newBuilder().setMargins(margins).build();

    MarginLayoutParams params =
        new MarginLayoutParams(MarginLayoutParams.MATCH_PARENT, MarginLayoutParams.WRAP_CONTENT);

    new StyleProvider(marginStyle).setMargins(context, params);
    verifyMargins(params, margins);
  }

  @Test
  public void testUiElementStyles_padding() {
    EdgeWidths padding =
        EdgeWidths.newBuilder().setTop(1).setBottom(2).setStart(3).setEnd(4).build();
    Style paddingStyle = Style.newBuilder().setPadding(padding).build();
    View view = new View(context);

    new StyleProvider(paddingStyle).setElementStyles(context, frameContext, view);
    verifyPadding(view, padding);
  }

  @Test
  public void testUiElementStyles_backgroundAndCorners() {
    View view = new View(context);

    int color = 0xffffffff;
    Fill background = Fill.newBuilder().setColor(color).build();
    RoundedCorners corners = RoundedCorners.newBuilder().setRadius(4).setBitmask(3).build();

    Style style =
        Style.newBuilder()
            .setColor(color)
            .setBackground(background)
            .setRoundedCorners(corners)
            .build();

    StyleProvider styleProvider = new StyleProvider(style);
    styleProvider.setElementStyles(context, frameContext, view);

    Drawable drawable = frameContext.createBackground(styleProvider, context);

    assertThat(view.getBackground()).isEqualTo(drawable);
  }

  @Test
  public void testUiElementStyles_noBackgroundInStyle() {
    View view = new View(context);
    view.setBackground(new ColorDrawable(0xffff0000));

    new StyleProvider(Style.getDefaultInstance()).setElementStyles(context, frameContext, view);
    assertThat(view.getBackground()).isNull();
  }

  @Test
  public void testUiElementStyles_noColorInFill() {
    View view = new View(context);
    view.setBackground(new ColorDrawable(0xffff0000));

    Style style = Style.newBuilder().setBackground(Fill.getDefaultInstance()).build();

    new StyleProvider(style).setElementStyles(context, frameContext, view);

    assertThat(view.getBackground()).isNull();
  }

  @Test
  public void testUiElementStyles_minimumHeight() {
    int minHeight = 12345;
    Style heightStyle = Style.newBuilder().setMinHeight(minHeight).build();
    View view = new View(context);

    new StyleProvider(heightStyle).setElementStyles(context, frameContext, view);

    assertThat(view.getMinimumHeight()).isEqualTo(minHeight);
  }

  @Test
  public void testUiElementStyles_noMinimumHeight() {
    Style noHeightStyle = Style.getDefaultInstance();
    View view = new View(context);
    view.setMinimumHeight(12345);

    new StyleProvider(noHeightStyle).setElementStyles(context, frameContext, view);

    assertThat(view.getMinimumHeight()).isEqualTo(0);
  }

  private void verifyPadding(View view, EdgeWidths padding) {
    assertThat(view.getPaddingTop()).isEqualTo((int) ViewUtils.dpToPx(padding.getTop(), context));
    assertThat(view.getPaddingBottom())
        .isEqualTo((int) ViewUtils.dpToPx(padding.getBottom(), context));
    assertThat(view.getPaddingStart())
        .isEqualTo((int) ViewUtils.dpToPx(padding.getStart(), context));
    assertThat(view.getPaddingEnd()).isEqualTo((int) ViewUtils.dpToPx(padding.getEnd(), context));
  }

  private void verifyMargins(MarginLayoutParams params, EdgeWidths margins) {
    assertThat(params.getMarginStart())
        .isEqualTo((int) ViewUtils.dpToPx(margins.getStart(), context));
    assertThat(params.getMarginEnd()).isEqualTo((int) ViewUtils.dpToPx(margins.getEnd(), context));
    assertThat(params.topMargin).isEqualTo((int) ViewUtils.dpToPx(margins.getTop(), context));
    assertThat(params.bottomMargin).isEqualTo((int) ViewUtils.dpToPx(margins.getBottom(), context));
    assertThat(params.leftMargin).isEqualTo((int) ViewUtils.dpToPx(margins.getStart(), context));
    assertThat(params.rightMargin).isEqualTo((int) ViewUtils.dpToPx(margins.getEnd(), context));
  }
}
