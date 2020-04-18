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
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.search.now.ui.piet.BindingRefsProto.StyleBindingRef;
import com.google.search.now.ui.piet.GradientsProto.ColorStop;
import com.google.search.now.ui.piet.GradientsProto.Fill;
import com.google.search.now.ui.piet.GradientsProto.LinearGradient;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import com.google.search.now.ui.piet.PietProto.Stylesheet;
import com.google.search.now.ui.piet.PietProto.Template;
import com.google.search.now.ui.piet.StylesProto.BoundStyle;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.Font.FontWeight;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link PietStylesHelper}. */
@RunWith(RobolectricTestRunner.class)
public class PietStylesHelperTest {

  private static final String STYLESHEET_ID_1 = "ss1";
  private static final String STYLESHEET_ID_2 = "ss2";
  private static final String STYLE_ID_1 = "s1";
  private static final String STYLE_ID_2 = "s2";
  private static final Style STYLE_1 = Style.newBuilder().setStyleId(STYLE_ID_1).build();
  private static final Style STYLE_2 = Style.newBuilder().setStyleId(STYLE_ID_2).build();
  private static final String TEMPLATE_ID_1 = "t1";
  private static final String TEMPLATE_ID_2 = "t2";
  private static final Template TEMPLATE_1 =
      Template.newBuilder().setTemplateId(TEMPLATE_ID_1).build();
  private static final Template TEMPLATE_2 =
      Template.newBuilder().setTemplateId(TEMPLATE_ID_2).build();

  private List<PietSharedState> sharedStates = new ArrayList<>();
  @Mock FrameContext frameContext;

  private PietStylesHelper helper;
  private PietSharedState sharedState1;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    Stylesheet.Builder stylesheet1 =
        Stylesheet.newBuilder().setStylesheetId(STYLESHEET_ID_1).addStyles(STYLE_1);

    Stylesheet.Builder stylesheet2 =
        Stylesheet.newBuilder().setStylesheetId(STYLESHEET_ID_2).addStyles(STYLE_2);

    sharedState1 =
        PietSharedState.newBuilder()
            .addStylesheets(stylesheet1.build())
            .addTemplates(TEMPLATE_1)
            .build();

    PietSharedState sharedState2 =
        PietSharedState.newBuilder()
            .addStylesheets(stylesheet2.build())
            .addTemplates(TEMPLATE_2)
            .build();
    sharedStates.add(sharedState1);
    sharedStates.add(sharedState2);

    helper = new PietStylesHelper(sharedStates);
  }

  @Test
  public void testGetStylesheet() {
    Map<String, Style> resultMap1 = helper.getStylesheet(STYLESHEET_ID_1);
    assertThat(resultMap1).containsExactly(STYLE_ID_1, STYLE_1);

    // Retrieve map again to test caching
    assertThat(helper.getStylesheet(STYLESHEET_ID_1)).isSameAs(resultMap1);

    Map<String, Style> resultMap2 = helper.getStylesheet(STYLESHEET_ID_2);
    assertThat(resultMap2).containsExactly(STYLE_ID_2, STYLE_2);
    assertThat(helper.getStylesheet(STYLESHEET_ID_2)).isSameAs(resultMap2);
  }

  @Test
  public void testGetStylesheet_withSameStylesheetId() {
    List<PietSharedState> sharedStates = new ArrayList<>();
    Stylesheet.Builder stylesheet =
        Stylesheet.newBuilder().setStylesheetId(STYLESHEET_ID_1).addStyles(STYLE_2);
    PietSharedState sharedState2 =
        PietSharedState.newBuilder()
            .addStylesheets(stylesheet.build())
            .addTemplates(TEMPLATE_2)
            .build();
    sharedStates.add(sharedState1);
    sharedStates.add(sharedState2);
    helper = new PietStylesHelper(sharedStates);

    Map<String, Style> resultMap = helper.getStylesheet(STYLESHEET_ID_1);
    assertThat(resultMap).containsExactly(STYLE_ID_2, STYLE_2);
  }

  @Test
  public void testGetStylesheet_notExist() {
    assertThat(helper.getStylesheet("NOT EXIST")).isEmpty();
  }

  @Test
  public void testCreateMapFromStylesheet() {
    Stylesheet.Builder myStyles = Stylesheet.newBuilder();
    Style sixties = Style.newBuilder().setStyleId("60s").build();
    myStyles.addStyles(sixties);
    Style eighties = Style.newBuilder().setStyleId("80s").build();
    myStyles.addStyles(eighties);

    assertThat(PietStylesHelper.createMapFromStylesheet(myStyles.build()))
        .containsExactly("60s", sixties, "80s", eighties);
  }

  @Test
  public void testGetTemplate() {
    assertThat(helper.getTemplate(TEMPLATE_ID_1)).isEqualTo(TEMPLATE_1);
    assertThat(helper.getTemplate(TEMPLATE_ID_2)).isEqualTo(TEMPLATE_2);
  }

  @Test
  public void testGetTemplate_notExist() {
    assertThat(helper.getTemplate("NOT EXIST")).isNull();
  }

  @Test
  public void testMergeStyleIdsStack() {
    // These constants are in the final output and are not overridden
    int baseWidth = 999;
    int style1Color = 12345;
    int style2MaxLines = 22222;
    int style2MinHeight = 33333;
    FontWeight style1FontWeight = FontWeight.MEDIUM;
    int style2FontSize = 13;
    int style1GradientColor = 1234;
    int style2GradientDirection = 321;
    int boundStyleColor = 5050;
    Fill boundStyleBackground =
        Fill.newBuilder()
            .setLinearGradient(LinearGradient.newBuilder().setDirectionDeg(boundStyleColor))
            .build();

    Style baseStyle =
        Style.newBuilder()
            .setWidth(baseWidth) // Not overridden
            .setColor(888) // Overridden
            .build();
    String styleId1 = "STYLE1";
    Style style1 =
        Style.newBuilder()
            .setColor(style1Color) // Not overridden
            .setMaxLines(54321) // Overridden
            .setFont(Font.newBuilder().setSize(11).setWeight(style1FontWeight))
            .setBackground(
                Fill.newBuilder()
                    .setLinearGradient(
                        LinearGradient.newBuilder()
                            .addStops(ColorStop.newBuilder().setColor(style1GradientColor))))
            .build();
    String styleId2 = "STYLE2";
    Style style2 =
        Style.newBuilder()
            .setMaxLines(style2MaxLines) // Overrides
            .setMinHeight(style2MinHeight) // Not an override
            .setFont(Font.newBuilder().setSize(style2FontSize)) // Just override size
            .setBackground(
                Fill.newBuilder()
                    .setLinearGradient(
                        LinearGradient.newBuilder().setDirectionDeg(style2GradientDirection)))
            .build();
    Map<String, Style> stylesheetMap = new HashMap<>();
    stylesheetMap.put(styleId1, style1);
    stylesheetMap.put(styleId2, style2);

    // Test merging a style IDs stack
    Style mergedStyle =
        PietStylesHelper.mergeStyleIdsStack(
            baseStyle,
            StyleIdsStack.newBuilder().addStyleIds(styleId1).addStyleIds(styleId2).build(),
            stylesheetMap,
            frameContext);

    assertThat(mergedStyle)
        .isEqualTo(
            Style.newBuilder()
                .setColor(style1Color)
                .setMaxLines(style2MaxLines)
                .setMinHeight(style2MinHeight)
                .setFont(Font.newBuilder().setSize(style2FontSize).setWeight(style1FontWeight))
                .setBackground(
                    Fill.newBuilder()
                        .setLinearGradient(
                            LinearGradient.newBuilder()
                                .addStops(ColorStop.newBuilder().setColor(style1GradientColor))
                                .setDirectionDeg(style2GradientDirection)))
                .setWidth(baseWidth)
                .build());

    // Test merging a style IDs stack with a bound style
    String styleBindingId = "BOUND_STYLE";
    BoundStyle boundStyle =
        BoundStyle.newBuilder()
            .setColor(boundStyleColor)
            .setBackground(boundStyleBackground)
            .build();
    StyleBindingRef styleBindingRef =
        StyleBindingRef.newBuilder().setBindingId(styleBindingId).build();
    when(frameContext.getStyleFromBinding(styleBindingRef)).thenReturn(boundStyle);

    mergedStyle =
        PietStylesHelper.mergeStyleIdsStack(
            baseStyle,
            StyleIdsStack.newBuilder()
                .addStyleIds(styleId1)
                .addStyleIds(styleId2)
                .setStyleBinding(styleBindingRef)
                .build(),
            stylesheetMap,
            frameContext);
    assertThat(mergedStyle)
        .isEqualTo(
            Style.newBuilder()
                .setColor(boundStyleColor)
                .setMaxLines(style2MaxLines)
                .setMinHeight(style2MinHeight)
                .setFont(Font.newBuilder().setSize(style2FontSize).setWeight(style1FontWeight))
                .setBackground(
                    Fill.newBuilder()
                        .setLinearGradient(
                            LinearGradient.newBuilder()
                                .addStops(ColorStop.newBuilder().setColor(style1GradientColor))
                                .setDirectionDeg(boundStyleColor)))
                .setWidth(baseWidth)
                .build());

    // Binding styles fails when frameContext is null.
    assertThatRunnable(
            () -> {
              PietStylesHelper.mergeStyleIdsStack(
                  Style.getDefaultInstance(),
                  StyleIdsStack.newBuilder()
                      .addStyleIds(styleId1)
                      .addStyleIds(styleId2)
                      .setStyleBinding(styleBindingRef)
                      .build(),
                  stylesheetMap,
                  null);
            })
        .throwsAnExceptionOfType(NullPointerException.class)
        .that()
        .hasMessageThat()
        .contains("Binding styles not supported when frameContext is null");
  }
}
