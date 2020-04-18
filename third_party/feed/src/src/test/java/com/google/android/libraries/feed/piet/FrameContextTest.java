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
import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE;
import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE_PROVIDER;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.Drawable;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.piet.DebugLogger.MessageType;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.android.libraries.feed.piet.ui.RoundedCornerColorDrawable;
import com.google.search.now.ui.piet.ActionsProto.Actions;
import com.google.search.now.ui.piet.BindingRefsProto.ActionsBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ChunkedTextBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.CustomBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ElementListBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.GridCellWidthBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ImageBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ParameterizedTextBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.StyleBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.TemplateBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingContext;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.CustomElementData;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.GravityVertical;
import com.google.search.now.ui.piet.ElementsProto.GridCellWidth;
import com.google.search.now.ui.piet.ElementsProto.HostBindingData;
import com.google.search.now.ui.piet.ElementsProto.TemplateInvocation;
import com.google.search.now.ui.piet.GradientsProto.ColorStop;
import com.google.search.now.ui.piet.GradientsProto.Fill;
import com.google.search.now.ui.piet.GradientsProto.LinearGradient;
import com.google.search.now.ui.piet.ImagesProto.Image;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import com.google.search.now.ui.piet.PietProto.Stylesheet;
import com.google.search.now.ui.piet.PietProto.Template;
import com.google.search.now.ui.piet.RoundedCornersProto.RoundedCorners;
import com.google.search.now.ui.piet.StylesProto.BoundStyle;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.Font.FontWeight;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import com.google.search.now.ui.piet.TextProto.Chunk;
import com.google.search.now.ui.piet.TextProto.ChunkedText;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import com.google.search.now.ui.piet.TextProto.StyledTextChunk;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link FrameContext}. */
@RunWith(RobolectricTestRunner.class)
public class FrameContextTest {
  private static final String DEFAULT_TEMPLATE_ID = "TEMPLATE_ID";
  private static final Template DEFAULT_TEMPLATE =
      Template.newBuilder().setTemplateId(DEFAULT_TEMPLATE_ID).build();
  private static final Frame DEFAULT_FRAME =
      Frame.newBuilder().addTemplates(DEFAULT_TEMPLATE).build();
  private static final String SAMPLE_STYLE_ID = "STYLE_ID";
  private static final int BASE_STYLE_COLOR = 111111;
  private static final int SAMPLE_STYLE_COLOR = 888888;
  private static final Style SAMPLE_STYLE =
      Style.newBuilder().setStyleId(SAMPLE_STYLE_ID).setColor(SAMPLE_STYLE_COLOR).build();
  private static final String STYLESHEET_ID = "STYLESHEET_ID";
  private static final Style BASE_STYLE = Style.newBuilder().setColor(BASE_STYLE_COLOR).build();
  private static final StyleIdsStack SAMPLE_STYLE_IDS =
      StyleIdsStack.newBuilder().addStyleIds(SAMPLE_STYLE_ID).build();
  private static final String BINDING_ID = "BINDING_ID";
  private static final String INVALID_BINDING_ID = "NOT_A_REAL_BINDING_ID";

  @Mock private AssetProvider assetProvider;
  @Mock private CustomElementProvider customElementProvider;
  @Mock private StyleProvider styleProvider;
  @Mock private ActionHandler actionHandler;

  private final Map<String, Style> defaultStylesheet = new HashMap<>();
  private final DebugLogger debugLogger = new DebugLogger();

  private Context context;
  private HostBindingProvider hostBindingProvider;
  private FrameContext frameContext;
  private List<PietSharedState> pietSharedStates;
  private PietStylesHelper pietStylesHelper;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);

    hostBindingProvider = new HostBindingProvider();

    defaultStylesheet.put(SAMPLE_STYLE_ID, SAMPLE_STYLE);
    pietSharedStates = new ArrayList<>();
    pietStylesHelper = new PietStylesHelper(pietSharedStates);
  }

  @Test
  public void testGetters() {
    String template2BindingId = "TEMPLATE_2";
    Template template2 = Template.newBuilder().setTemplateId(template2BindingId).build();
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template2).build();
    pietSharedStates.add(sharedState);
    frameContext =
        new FrameContext(
            DEFAULT_FRAME,
            defaultStylesheet,
            SAMPLE_STYLE,
            styleProvider,
            pietSharedStates,
            new PietStylesHelper(pietSharedStates),
            DebugBehavior.VERBOSE,
            debugLogger,
            assetProvider,
            customElementProvider,
            hostBindingProvider,
            actionHandler);

    assertThat(frameContext.getAssetProvider()).isEqualTo(assetProvider);
    assertThat(frameContext.getCustomElementProvider()).isEqualTo(customElementProvider);
    assertThat(frameContext.getFrame()).isEqualTo(DEFAULT_FRAME);
    assertThat(frameContext.getCurrentStyle()).isEqualTo(styleProvider);
    assertThat(frameContext.getActionHandler()).isEqualTo(actionHandler);

    assertThat(frameContext.getTemplate(DEFAULT_TEMPLATE_ID)).isEqualTo(DEFAULT_TEMPLATE);

    assertThat(frameContext.getTemplate(template2BindingId)).isEqualTo(template2);
  }

  @Test
  public void testThrowsWithNoBindingContext() {
    frameContext = makeFrameContextWithNoBindings();

    assertThatRunnable(
            () -> frameContext.getActionsFromBinding(ActionsBindingRef.getDefaultInstance()))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("no BindingValues defined");
  }

  @Test
  public void testBindTemplate_lotsOfStuff() {
    frameContext = defaultFrameContext();

    // null binding context returns the same frame context.
    assertThat(frameContext.bindTemplate(DEFAULT_TEMPLATE, null)).isEqualTo(frameContext);

    // bindTemplate should add new BindingValues
    ParameterizedText text = ParameterizedText.newBuilder().setText("Calico").build();
    BindingValue textBinding =
        BindingValue.newBuilder().setBindingId(BINDING_ID).setParameterizedText(text).build();
    FrameContext frameContextWithBindings =
        frameContext.bindTemplate(
            DEFAULT_TEMPLATE, BindingContext.newBuilder().addBindingValues(textBinding).build());
    assertThat(
            frameContextWithBindings
                .getParameterizedTextBindingValue(
                    ParameterizedTextBindingRef.newBuilder().setBindingId(BINDING_ID).build())
                .getParameterizedText())
        .isEqualTo(text);

    // and clear out all the previous styles
    assertThat(frameContextWithBindings.getCurrentStyle()).isEqualTo(DEFAULT_STYLE_PROVIDER);
    assertThat(frameContextWithBindings.makeStyleFor(SAMPLE_STYLE_IDS).getColor())
        .isEqualTo(DEFAULT_STYLE_PROVIDER.getColor());

    // A local template stylesheet should override the previous styles.
    // Look up a style from a template's local stylesheet, and check the default style.
    int templateStyleColor = 343434;
    int templateDefaultColor = 434343;
    Template templateWithStyles =
        Template.newBuilder()
            .setStylesheet(
                Stylesheet.newBuilder()
                    .addStyles(
                        Style.newBuilder().setStyleId("templateStyle").setColor(templateStyleColor))
                    .addStyles(
                        Style.newBuilder()
                            .setStyleId("templateDefault")
                            .setColor(templateDefaultColor)))
            .setChildDefaultStyleIds(StyleIdsStack.newBuilder().addStyleIds("templateDefault"))
            .build();
    FrameContext frameContextWithTemplate =
        frameContext.bindTemplate(templateWithStyles, BindingContext.getDefaultInstance());
    assertThat(
            frameContextWithTemplate
                .makeStyleFor(StyleIdsStack.newBuilder().addStyleIds("templateStyle").build())
                .getColor())
        .isEqualTo(templateStyleColor);
    assertThat(frameContextWithTemplate.getCurrentStyle().getColor())
        .isEqualTo(templateDefaultColor);

    // Can't look up the SAMPLE_STYLE_ID anymore, so we return the default.
    assertThat(frameContextWithTemplate.makeStyleFor(SAMPLE_STYLE_IDS).getColor())
        .isEqualTo(templateDefaultColor);
  }

  @Test
  public void testBindNewStyle() {
    frameContext = defaultFrameContext();

    FrameContext noStyleFrameContext =
        frameContext.bindNewStyle(StyleIdsStack.getDefaultInstance());
    assertThat(noStyleFrameContext).isNotSameAs(frameContext);
    assertThat(noStyleFrameContext.getCurrentStyle().getColor()).isEqualTo(BASE_STYLE_COLOR);

    FrameContext frameContextWithStyle = frameContext.bindNewStyle(SAMPLE_STYLE_IDS);
    assertThat(frameContextWithStyle).isNotEqualTo(frameContext);
    assertThat(frameContextWithStyle.getCurrentStyle().getColor()).isEqualTo(SAMPLE_STYLE_COLOR);
    assertThat(frameContext.getCurrentStyle().getColor()).isNotEqualTo(SAMPLE_STYLE_COLOR);
  }

  @Test
  public void testMakeStyleFor() {
    frameContext = defaultFrameContext();

    // Returns base style provider if there are no styles defined
    StyleProvider noStyle = frameContext.makeStyleFor(StyleIdsStack.getDefaultInstance());
    assertThat(noStyle.getColor()).isEqualTo(BASE_STYLE_COLOR);

    // Successful lookup results in a new style provider
    StyleProvider defaultStyle = frameContext.makeStyleFor(SAMPLE_STYLE_IDS);
    assertThat(defaultStyle.getColor()).isEqualTo(SAMPLE_STYLE_COLOR);

    // Failed lookup returns the current style provider
    StyleProvider notFoundStyle =
        frameContext.makeStyleFor(
            StyleIdsStack.newBuilder().addStyleIds(INVALID_BINDING_ID).build());
    assertThat(notFoundStyle.getColor()).isEqualTo(BASE_STYLE_COLOR);
  }

  @Test
  public void testCreateBackground() {
    int color = 12345;
    Fill fill = Fill.newBuilder().setColor(color).build();
    RoundedCorners roundedCorners = RoundedCorners.newBuilder().setRadius(4321).build();
    when(styleProvider.getRoundedCorners()).thenReturn(roundedCorners);

    frameContext = defaultFrameContext();

    when(styleProvider.getBackground()).thenReturn(Fill.getDefaultInstance());
    assertThat(frameContext.createBackground(styleProvider, context)).isNull();

    when(styleProvider.getBackground()).thenReturn(fill);
    Drawable backgroundDrawable = frameContext.createBackground(styleProvider, context);
    assertThat(backgroundDrawable).isInstanceOf(RoundedCornerColorDrawable.class);

    RoundedCornerColorDrawable background = (RoundedCornerColorDrawable) backgroundDrawable;
    assertThat(background.getColor()).isEqualTo(color);

    // TODO: how to test rounded corners?
  }

  @Test
  public void testGetText() {
    ParameterizedText text = ParameterizedText.newBuilder().setText("tabby").build();
    BindingValue textBindingValue = defaultBinding().setParameterizedText(text).build();
    ParameterizedTextBindingRef textBindingRef =
        ParameterizedTextBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(textBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getParameterizedTextBindingValue(textBindingRef))
        .isEqualTo(textBindingValue);

    // Can't look up binding
    assertThatRunnable(
            () ->
                frameContext.getParameterizedTextBindingValue(
                    ParameterizedTextBindingRef.newBuilder()
                        .setBindingId(INVALID_BINDING_ID)
                        .build()))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Parameterized text binding not found");

    // Binding has no content
    assertThatRunnable(
            () ->
                makeFrameContextWithEmptyBinding().getParameterizedTextBindingValue(textBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Parameterized text binding not found");

    // Binding has no content but is GONE
    textBindingValue = defaultBinding().setVisibility(Visibility.GONE).build();
    frameContext = makeFrameContextWithBinding(textBindingValue);
    assertThatRunnable(() -> frameContext.getParameterizedTextBindingValue(textBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Parameterized text binding not found");

    // Binding is invalid but is optional
    ParameterizedTextBindingRef textBindingRefInvalidOptional =
        ParameterizedTextBindingRef.newBuilder()
            .setBindingId(INVALID_BINDING_ID)
            .setIsOptional(true)
            .build();
    assertThat(
            makeFrameContextWithEmptyBinding()
                .getParameterizedTextBindingValue(textBindingRefInvalidOptional))
        .isEqualTo(BindingValue.getDefaultInstance());

    // Binding has no content but is optional
    ParameterizedTextBindingRef textBindingRefOptional =
        ParameterizedTextBindingRef.newBuilder()
            .setBindingId(BINDING_ID)
            .setIsOptional(true)
            .build();
    assertThat(
            makeFrameContextWithEmptyBinding()
                .getParameterizedTextBindingValue(textBindingRefOptional))
        .isEqualTo(BindingValue.getDefaultInstance());
  }

  @Test
  public void testGetText_hostBinding() {
    ParameterizedText text = ParameterizedText.newBuilder().setText("tabby").build();
    BindingValue textBindingValue = defaultBinding().setParameterizedText(text).build();
    BindingValue hostTextBindingValue =
        defaultBinding()
            .setHostBindingData(HostBindingData.newBuilder())
            .setParameterizedText(text)
            .build();
    ParameterizedTextBindingRef textBindingRef =
        ParameterizedTextBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(hostTextBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getParameterizedTextBindingValue(textBindingRef))
        .isEqualTo(textBindingValue);
  }

  @Test
  public void testGetImage() {
    Image image = Image.newBuilder().setTintColor(12345).build();
    BindingValue imageBindingValue = defaultBinding().setImage(image).build();
    ImageBindingRef imageBindingRef = ImageBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(imageBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getImageBindingValue(imageBindingRef)).isEqualTo(imageBindingValue);

    // Can't look up binding
    assertThatRunnable(
            () ->
                frameContext.getImageBindingValue(
                    ImageBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).build()))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Image binding not found");

    // Binding has no content
    assertThatRunnable(
            () -> makeFrameContextWithEmptyBinding().getImageBindingValue(imageBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Image binding not found");

    // Binding has no content but is GONE
    imageBindingValue = defaultBinding().setVisibility(Visibility.GONE).build();
    frameContext = makeFrameContextWithBinding(imageBindingValue);
    assertThatRunnable(() -> frameContext.getImageBindingValue(imageBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Image binding not found");

    // Binding is invalid but is optional
    ImageBindingRef imageBindingRefInvalidOptional =
        ImageBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).setIsOptional(true).build();
    assertThat(
            makeFrameContextWithEmptyBinding().getImageBindingValue(imageBindingRefInvalidOptional))
        .isEqualTo(BindingValue.getDefaultInstance());

    // Binding has no content but is optional
    ImageBindingRef imageBindingRefOptional =
        ImageBindingRef.newBuilder().setBindingId(BINDING_ID).setIsOptional(true).build();
    assertThat(makeFrameContextWithEmptyBinding().getImageBindingValue(imageBindingRefOptional))
        .isEqualTo(BindingValue.getDefaultInstance());
  }

  @Test
  public void testGetImage_hostBinding() {
    Image image = Image.newBuilder().setTintColor(12345).build();
    BindingValue imageBindingValue = defaultBinding().setImage(image).build();
    BindingValue hostImageBindingValue =
        defaultBinding().setHostBindingData(HostBindingData.newBuilder()).setImage(image).build();
    ImageBindingRef imageBindingRef = ImageBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(hostImageBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getImageBindingValue(imageBindingRef)).isEqualTo(imageBindingValue);
  }

  @Test
  public void testGetElementList() {
    ElementList list =
        ElementList.newBuilder().setGravityVertical(GravityVertical.GRAVITY_MIDDLE).build();
    BindingValue listBindingValue = defaultBinding().setElementList(list).build();
    ElementListBindingRef listBindingRef =
        ElementListBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(listBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getElementListBindingValue(listBindingRef)).isEqualTo(listBindingValue);

    // Can't look up binding
    assertThatRunnable(
            () ->
                frameContext.getElementListBindingValue(
                    ElementListBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).build()))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("ElementList binding not found");

    // Binding has no content
    assertThatRunnable(
            () -> makeFrameContextWithEmptyBinding().getElementListBindingValue(listBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("ElementList binding not found");

    // Binding has no content but is GONE
    listBindingValue = defaultBinding().setVisibility(Visibility.GONE).build();
    frameContext = makeFrameContextWithBinding(listBindingValue);
    assertThatRunnable(() -> frameContext.getElementListBindingValue(listBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("ElementList binding not found");

    // Binding is missine but is optional
    ElementListBindingRef listBindingRefInvalidOptional =
        ElementListBindingRef.newBuilder()
            .setBindingId(INVALID_BINDING_ID)
            .setIsOptional(true)
            .build();
    assertThat(
            makeFrameContextWithEmptyBinding()
                .getElementListBindingValue(listBindingRefInvalidOptional))
        .isEqualTo(BindingValue.getDefaultInstance());

    // Binding has no content but is optional
    ElementListBindingRef listBindingRefOptional =
        ElementListBindingRef.newBuilder().setBindingId(BINDING_ID).setIsOptional(true).build();
    assertThat(
            makeFrameContextWithEmptyBinding().getElementListBindingValue(listBindingRefOptional))
        .isEqualTo(BindingValue.getDefaultInstance());
  }

  @Test
  public void testGetElementList_hostBinding() {
    ElementList list =
        ElementList.newBuilder().setGravityVertical(GravityVertical.GRAVITY_MIDDLE).build();
    BindingValue listBindingValue = defaultBinding().setElementList(list).build();
    BindingValue hostListBindingValue =
        defaultBinding()
            .setHostBindingData(HostBindingData.newBuilder())
            .setElementList(list)
            .build();
    ElementListBindingRef listBindingRef =
        ElementListBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(hostListBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getElementListBindingValue(listBindingRef)).isEqualTo(listBindingValue);
  }

  @Test
  public void testGetGridCellWidthFromBinding() {
    GridCellWidth cellWidth = GridCellWidth.newBuilder().setWeight(123).build();
    frameContext = makeFrameContextWithBinding(defaultBinding().setCellWidth(cellWidth).build());
    assertThat(
            frameContext.getGridCellWidthFromBinding(
                GridCellWidthBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isEqualTo(cellWidth);
    assertThat(
            frameContext.getGridCellWidthFromBinding(
                GridCellWidthBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).build()))
        .isNull();

    frameContext = makeFrameContextWithEmptyBinding();
    assertThat(
            frameContext.getGridCellWidthFromBinding(
                GridCellWidthBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isNull();
  }

  @Test
  public void testGetGridCellWidthFromBinding_hostBinding() {
    GridCellWidth cellWidth = GridCellWidth.newBuilder().setWeight(123).build();
    frameContext =
        makeFrameContextWithBinding(
            defaultBinding()
                .setHostBindingData(HostBindingData.newBuilder())
                .setCellWidth(cellWidth)
                .build());
    assertThat(
            frameContext.getGridCellWidthFromBinding(
                GridCellWidthBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isEqualTo(cellWidth);
  }

  @Test
  public void testGetActionsFromBinding() {
    Actions actions = Actions.newBuilder().build();
    frameContext = makeFrameContextWithBinding(defaultBinding().setActions(actions).build());
    assertThat(
            frameContext.getActionsFromBinding(
                ActionsBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isSameAs(actions);
    assertThat(
            frameContext.getActionsFromBinding(
                ActionsBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).build()))
        .isSameAs(Actions.getDefaultInstance());

    frameContext = makeFrameContextWithEmptyBinding();
    assertThat(
            frameContext.getActionsFromBinding(
                ActionsBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isSameAs(Actions.getDefaultInstance());
  }

  @Test
  public void testGetActionsFromBinding_hostBinding() {
    frameContext =
        makeFrameContextWithBinding(
            defaultBinding()
                .setHostBindingData(HostBindingData.newBuilder())
                .setActions(Actions.getDefaultInstance())
                .build());
    assertThat(
            frameContext.getActionsFromBinding(
                ActionsBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isEqualTo(Actions.getDefaultInstance());
  }

  @Test
  public void testGetStyleFromBinding() {
    BoundStyle boundStyle = BoundStyle.newBuilder().setColor(12345).build();
    frameContext = makeFrameContextWithBinding(defaultBinding().setBoundStyle(boundStyle).build());
    assertThat(
            frameContext.getStyleFromBinding(
                StyleBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isEqualTo(boundStyle);
    assertThat(
            frameContext.getStyleFromBinding(
                StyleBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).build()))
        .isEqualTo(BoundStyle.getDefaultInstance());

    frameContext = makeFrameContextWithEmptyBinding();
    assertThat(
            frameContext.getStyleFromBinding(
                StyleBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isEqualTo(BoundStyle.getDefaultInstance());
  }

  @Test
  public void testGetStyleFromBinding_hostBinding() {
    BoundStyle boundStyle = BoundStyle.newBuilder().setColor(12345).build();
    frameContext =
        makeFrameContextWithBinding(
            defaultBinding()
                .setHostBindingData(HostBindingData.newBuilder())
                .setBoundStyle(boundStyle)
                .build());
    assertThat(
            frameContext.getStyleFromBinding(
                StyleBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isEqualTo(boundStyle);
  }

  @Test
  public void testGetTemplateInvocationFromBinding() {
    TemplateInvocation templateInvocation =
        TemplateInvocation.newBuilder().setTemplateId("carboncopy").build();
    frameContext =
        makeFrameContextWithBinding(
            defaultBinding().setTemplateInvocation(templateInvocation).build());
    assertThat(
            frameContext.getTemplateInvocationFromBinding(
                TemplateBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isEqualTo(templateInvocation);
    assertThat(
            frameContext.getTemplateInvocationFromBinding(
                TemplateBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).build()))
        .isNull();

    frameContext = makeFrameContextWithEmptyBinding();
    assertThat(
            frameContext.getTemplateInvocationFromBinding(
                TemplateBindingRef.newBuilder().setBindingId(BINDING_ID).build()))
        .isNull();
  }

  @Test
  public void testGetTemplateInvocationFromBinding_hostBinding() {
    TemplateInvocation templateInvocation =
        TemplateInvocation.newBuilder().setTemplateId("carboncopy").build();
    frameContext =
        makeFrameContextWithBinding(
            defaultBinding()
                .setHostBindingData(HostBindingData.newBuilder())
                .setTemplateInvocation(templateInvocation)
                .build());
  }

  @Test
  public void testGetCustomElementBindingValue() {
    CustomElementData customElement = CustomElementData.getDefaultInstance();
    BindingValue customElementBindingValue =
        defaultBinding().setCustomElementData(customElement).build();
    CustomBindingRef customBindingRef =
        CustomBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(customElementBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getCustomElementBindingValue(customBindingRef))
        .isEqualTo(customElementBindingValue);

    // Can't look up binding
    assertThatRunnable(
            () ->
                frameContext.getCustomElementBindingValue(
                    CustomBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).build()))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Custom element binding not found");

    // Binding has no content
    assertThatRunnable(
            () -> makeFrameContextWithEmptyBinding().getCustomElementBindingValue(customBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Custom element binding not found");

    // Binding has no content but is GONE
    customElementBindingValue = defaultBinding().setVisibility(Visibility.GONE).build();
    frameContext = makeFrameContextWithBinding(customElementBindingValue);
    assertThatRunnable(() -> frameContext.getCustomElementBindingValue(customBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Custom element binding not found");

    // Binding is missing but is optional
    CustomBindingRef customBindingRefInvalidOptional =
        CustomBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).setIsOptional(true).build();
    assertThat(
            makeFrameContextWithEmptyBinding()
                .getCustomElementBindingValue(customBindingRefInvalidOptional))
        .isEqualTo(BindingValue.getDefaultInstance());

    // Binding has no content but is optional
    CustomBindingRef customBindingRefOptional =
        CustomBindingRef.newBuilder().setBindingId(BINDING_ID).setIsOptional(true).build();
    assertThat(
            makeFrameContextWithEmptyBinding()
                .getCustomElementBindingValue(customBindingRefOptional))
        .isEqualTo(BindingValue.getDefaultInstance());
  }

  @Test
  public void testGetCustomElementBindingValue_hostBinding() {
    CustomElementData customElement = CustomElementData.getDefaultInstance();
    BindingValue customElementBindingValue =
        defaultBinding().setCustomElementData(customElement).build();
    BindingValue hostCustomElementBindingValue =
        defaultBinding()
            .setHostBindingData(HostBindingData.newBuilder())
            .setCustomElementData(customElement)
            .build();
    CustomBindingRef customBindingRef =
        CustomBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(hostCustomElementBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getCustomElementBindingValue(customBindingRef))
        .isEqualTo(customElementBindingValue);
  }

  @Test
  public void testGetChunkedTextBindingValue() {
    ChunkedText text =
        ChunkedText.newBuilder()
            .addChunks(
                Chunk.newBuilder()
                    .setTextChunk(
                        StyledTextChunk.newBuilder()
                            .setParameterizedText(ParameterizedText.newBuilder().setText("text"))))
            .build();
    BindingValue textBindingValue = defaultBinding().setChunkedText(text).build();
    ChunkedTextBindingRef textBindingRef =
        ChunkedTextBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(textBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getChunkedTextBindingValue(textBindingRef)).isEqualTo(textBindingValue);

    // Can't look up binding
    assertThatRunnable(
            () ->
                frameContext.getChunkedTextBindingValue(
                    ChunkedTextBindingRef.newBuilder().setBindingId(INVALID_BINDING_ID).build()))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Chunked text binding not found");

    // Binding has no content
    assertThatRunnable(
            () -> makeFrameContextWithEmptyBinding().getChunkedTextBindingValue(textBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Chunked text binding not found");

    // Binding has no content but is GONE
    textBindingValue = defaultBinding().setVisibility(Visibility.GONE).build();
    frameContext = makeFrameContextWithBinding(textBindingValue);
    assertThatRunnable(() -> frameContext.getChunkedTextBindingValue(textBindingRef))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Chunked text binding not found");

    // Binding is missing but is optional
    ChunkedTextBindingRef textBindingRefInvalidOptional =
        ChunkedTextBindingRef.newBuilder()
            .setBindingId(INVALID_BINDING_ID)
            .setIsOptional(true)
            .build();
    assertThat(
            makeFrameContextWithEmptyBinding()
                .getChunkedTextBindingValue(textBindingRefInvalidOptional))
        .isEqualTo(BindingValue.getDefaultInstance());

    // Binding has no content but is optional
    ChunkedTextBindingRef textBindingRefOptional =
        ChunkedTextBindingRef.newBuilder().setBindingId(BINDING_ID).setIsOptional(true).build();
    assertThat(
            makeFrameContextWithEmptyBinding().getChunkedTextBindingValue(textBindingRefOptional))
        .isEqualTo(BindingValue.getDefaultInstance());
  }

  @Test
  public void testGetChunkedTextBindingValue_hostBinding() {
    ChunkedText text =
        ChunkedText.newBuilder()
            .addChunks(
                Chunk.newBuilder()
                    .setTextChunk(
                        StyledTextChunk.newBuilder()
                            .setParameterizedText(ParameterizedText.newBuilder().setText("text"))))
            .build();
    BindingValue textBindingValue = defaultBinding().setChunkedText(text).build();
    BindingValue hostTextBindingValue =
        defaultBinding()
            .setHostBindingData(HostBindingData.newBuilder())
            .setChunkedText(text)
            .build();
    ChunkedTextBindingRef textBindingRef =
        ChunkedTextBindingRef.newBuilder().setBindingId(BINDING_ID).build();

    frameContext = makeFrameContextWithBinding(hostTextBindingValue);

    // Succeed in looking up binding
    assertThat(frameContext.getChunkedTextBindingValue(textBindingRef)).isEqualTo(textBindingValue);
  }

  @Test
  public void testError_noStylesheet() {
    Frame frame = Frame.getDefaultInstance();
    FrameContext frameContext = makeFrameContextFromFrame(frame);
    assertThat(frameContext).isNotNull();
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).hasSize(1);
  }

  @Test
  public void testError_stylesheetNotFound() {
    Frame frame = getBaseFrame();
    FrameContext frameContext = makeFrameContextFromFrame(frame);
    assertThat(frameContext).isNotNull();
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).hasSize(1);
  }

  @Test
  public void testError_styleNotFound() {
    Frame frame = getBaseFrame();
    FrameContext frameContext = makeFrameContextFromFrame(frame);
    assertThat(frameContext).isNotNull();
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).hasSize(1);
  }

  @Test
  public void testCreateWithoutError() {
    Frame frame = getWorkingFrame();
    FrameContext frameContext = makeFrameContextFromFrame(frame);
    assertThat(frameContext).isNotNull();
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).isEmpty();
  }

  @Test
  public void testBindFrame_withStylesheetId() {
    Frame frame = Frame.newBuilder().setStylesheetId(STYLESHEET_ID).build();
    setUpPietSharedStates();
    FrameContext frameContext = makeFrameContextFromFrame(frame);

    // The style is not currently bound, but available from the stylesheet.
    assertThat(frameContext.getCurrentStyle().getColor()).isNotEqualTo(SAMPLE_STYLE_COLOR);
    assertThat(frameContext.makeStyleFor(SAMPLE_STYLE_IDS).getColor())
        .isEqualTo(SAMPLE_STYLE_COLOR);
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).isEmpty();
  }

  @Test
  public void testBindFrame_withStylesheet() {
    Frame frame =
        Frame.newBuilder().setStylesheet(Stylesheet.newBuilder().addStyles(SAMPLE_STYLE)).build();

    FrameContext frameContext = makeFrameContextFromFrame(frame);

    // The style is not currently bound, but available from the stylesheet.
    assertThat(frameContext.getCurrentStyle().getColor()).isNotEqualTo(SAMPLE_STYLE_COLOR);
    assertThat(frameContext.makeStyleFor(SAMPLE_STYLE_IDS).getColor())
        .isEqualTo(SAMPLE_STYLE_COLOR);
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).isEmpty();
  }

  @Test
  public void testBindFrame_withFrameStyle() {
    Frame frame =
        Frame.newBuilder()
            .setStylesheet(Stylesheet.newBuilder().addStyles(SAMPLE_STYLE))
            .setStyleReferences(SAMPLE_STYLE_IDS)
            .build();

    FrameContext frameContext = makeFrameContextFromFrame(frame);

    assertThat(frameContext.getCurrentStyle().getColor()).isEqualTo(SAMPLE_STYLE_COLOR);
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).isEmpty();
  }

  @Test
  public void testBindFrame_withoutFrameStyle() {
    Frame frame =
        Frame.newBuilder().setStylesheet(Stylesheet.newBuilder().addStyles(SAMPLE_STYLE)).build();

    FrameContext frameContext = makeFrameContextFromFrame(frame);

    assertThat(frameContext.getCurrentStyle()).isSameAs(DEFAULT_STYLE_PROVIDER);
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).isEmpty();
  }

  @Test
  public void testBindFrame_baseStyle() {
    int styleHeight = 747;
    String heightStyleId = "JUMBO";
    Frame frame =
        Frame.newBuilder()
            .setStylesheet(
                Stylesheet.newBuilder()
                    .addStyles(SAMPLE_STYLE)
                    .addStyles(Style.newBuilder().setStyleId(heightStyleId).setHeight(styleHeight)))
            .setStyleReferences(SAMPLE_STYLE_IDS)
            .build();

    // Set up a frame with a color applied to the frame.
    FrameContext frameContext = makeFrameContextFromFrame(frame);
    StyleProvider baseStyleWithHeight =
        frameContext.makeStyleFor(StyleIdsStack.newBuilder().addStyleIds(heightStyleId).build());

    // Make a style for something that doesn't override color, and check that the base color is a
    // default, not the frame color.
    assertThat(baseStyleWithHeight.getColor()).isEqualTo(DEFAULT_STYLE.getColor());
    assertThat(baseStyleWithHeight.getHeight()).isEqualTo(styleHeight);
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).isEmpty();
  }

  @Test
  public void testMergeStyleIdsStack() {
    String styleId1 = "STYLE1";
    Style style1 =
        Style.newBuilder()
            .setColor(12345) // Not overridden
            .setMaxLines(54321) // Overridden
            .setFont(Font.newBuilder().setSize(11).setWeight(FontWeight.MEDIUM))
            .setBackground(
                Fill.newBuilder()
                    .setLinearGradient(
                        LinearGradient.newBuilder()
                            .addStops(ColorStop.newBuilder().setColor(1234))))
            .setStyleId(styleId1)
            .build();
    String styleId2 = "STYLE2";
    Style style2 =
        Style.newBuilder()
            .setMaxLines(22222) // Overrides
            .setMinHeight(33333) // Not an override
            .setFont(Font.newBuilder().setSize(13))
            .setBackground(
                Fill.newBuilder()
                    .setLinearGradient(LinearGradient.newBuilder().setDirectionDeg(321)))
            .setStyleId(styleId2)
            .build();
    Frame frame =
        Frame.newBuilder()
            .setStylesheetId(STYLESHEET_ID)
            .setStyleReferences(
                StyleIdsStack.newBuilder().addStyleIds(styleId1).addStyleIds(styleId2))
            .build();
    pietSharedStates.add(
        PietSharedState.newBuilder()
            .addStylesheets(
                Stylesheet.newBuilder()
                    .setStylesheetId(STYLESHEET_ID)
                    .addStyles(style1)
                    .addStyles(style2)
                    .build())
            .build());

    FrameContext frameContext = makeFrameContextFromFrame(frame);

    assertThat(frameContext.getCurrentStyle().getColor()).isEqualTo(12345);
    assertThat(frameContext.getCurrentStyle().getMaxLines()).isEqualTo(22222);
    assertThat(frameContext.getCurrentStyle().getMinHeight()).isEqualTo(33333);
    assertThat(frameContext.getCurrentStyle().getFont())
        .isEqualTo(Font.newBuilder().setSize(13).setWeight(FontWeight.MEDIUM).build());
    assertThat(frameContext.getCurrentStyle().getBackground())
        .isEqualTo(
            Fill.newBuilder()
                .setLinearGradient(
                    LinearGradient.newBuilder()
                        .addStops(ColorStop.newBuilder().setColor(1234))
                        .setDirectionDeg(321))
                .build());
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).isEmpty();
  }

  @Test
  public void testError_bindingTemplate() {
    FrameContext frameContext = makeFrameContextFromFrame(getWorkingFrame());
    Template template = Template.getDefaultInstance();
    FrameContext boundCardContext2 = frameContext.bindTemplate(template, null);
    assertThat(boundCardContext2).isEqualTo(frameContext);
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.WARNING)).hasSize(1);
    assertThat(frameContext.getDebugLogger().getMessages(MessageType.ERROR)).isEmpty();
  }

  private FrameContext defaultFrameContext() {
    return makeFrameContextWithNoBindings();
  }

  private FrameContext makeFrameContextWithNoBindings() {
    return new FrameContext(
        DEFAULT_FRAME,
        defaultStylesheet,
        BASE_STYLE,
        styleProvider,
        pietSharedStates,
        new PietStylesHelper(pietSharedStates),
        DebugBehavior.VERBOSE,
        debugLogger,
        assetProvider,
        customElementProvider,
        hostBindingProvider,
        actionHandler);
  }

  private FrameContext makeFrameContextWithBinding(BindingValue bindingValue) {
    Map<String, BindingValue> bindingValueMap = new HashMap<>();
    bindingValueMap.put(BINDING_ID, bindingValue);
    return new FrameContext(
        DEFAULT_FRAME,
        defaultStylesheet,
        bindingValueMap,
        BASE_STYLE,
        styleProvider,
        pietSharedStates,
        pietStylesHelper,
        DebugBehavior.VERBOSE,
        debugLogger,
        assetProvider,
        customElementProvider,
        hostBindingProvider,
        actionHandler);
  }

  private FrameContext makeFrameContextWithEmptyBinding() {
    Map<String, BindingValue> bindingValueMap = new HashMap<>();
    bindingValueMap.put(BINDING_ID, BindingValue.getDefaultInstance());
    return new FrameContext(
        DEFAULT_FRAME,
        defaultStylesheet,
        bindingValueMap,
        BASE_STYLE,
        styleProvider,
        pietSharedStates,
        pietStylesHelper,
        DebugBehavior.VERBOSE,
        debugLogger,
        assetProvider,
        customElementProvider,
        hostBindingProvider,
        actionHandler);
  }

  private FrameContext makeFrameContextFromFrame(Frame frame) {
    return FrameContext.createFrameContext(
        frame,
        DEFAULT_STYLE,
        pietSharedStates,
        DebugBehavior.VERBOSE,
        debugLogger,
        assetProvider,
        customElementProvider,
        hostBindingProvider,
        actionHandler);
  }

  private void setUpPietSharedStates() {
    pietSharedStates.add(
        PietSharedState.newBuilder()
            .addStylesheets(
                Stylesheet.newBuilder().setStylesheetId(STYLESHEET_ID).addStyles(SAMPLE_STYLE))
            .build());
  }

  private Frame getWorkingFrame() {
    setUpPietSharedStates();
    return getBaseFrame();
  }

  private Frame getBaseFrame() {
    return Frame.newBuilder()
        .setStylesheetId(STYLESHEET_ID)
        .setStyleReferences(SAMPLE_STYLE_IDS)
        .build();
  }

  private BindingValue.Builder defaultBinding() {
    return BindingValue.newBuilder().setBindingId(BINDING_ID);
  }
}
