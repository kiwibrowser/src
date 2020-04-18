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
import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE_PROVIDER;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.piet.ImageElementAdapter.KeySupplier;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.search.now.ui.piet.BindingRefsProto.ImageBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.StyleBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.CustomElement;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ImageElement;
import com.google.search.now.ui.piet.ImagesProto.Image;
import com.google.search.now.ui.piet.ImagesProto.ImageSource;
import com.google.search.now.ui.piet.RoundedCornersProto.RoundedCorners;
import com.google.search.now.ui.piet.RoundedCornersProto.RoundedCorners.Corners;
import com.google.search.now.ui.piet.StylesProto.EdgeWidths;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link ImageElementAdapter}. */
@RunWith(RobolectricTestRunner.class)
public class ImageElementAdapterTest {
  private static final int HEIGHT_DP = 123;
  private static final int WIDTH_DP = 321;
  private static final EdgeWidths PADDING =
      EdgeWidths.newBuilder().setBottom(1).setTop(2).setStart(3).setEnd(4).build();
  private static final RoundedCorners CORNERS =
      RoundedCorners.newBuilder().setBitmask(Corners.BOTTOM_LEFT_VALUE).setRadius(34).build();
  private static final Image DEFAULT_IMAGE =
      Image.newBuilder().addSources(ImageSource.newBuilder().setUrl("icanhas.chz")).build();
  private static final ImageElement DEFAULT_MODEL =
      ImageElement.newBuilder().setImage(DEFAULT_IMAGE).build();

  @Mock private ElementAdapterFactory adapterFactory;
  @Mock private FrameContext frameContext;
  @Mock private FrameContext frameContextWithStyle;
  @Mock private AssetProvider assetProvider;
  @Mock private StyleProvider styleProvider;

  @Captor ArgumentCaptor<Consumer<Drawable>> consumerArgumentCaptor;

  private Context context;
  private int heightPx;
  private int widthPx;
  private ImageView imageView;
  private final Drawable defaultDrawable = new ColorDrawable(Color.BLUE);

  private ImageElementAdapter adapter;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    heightPx = (int) ViewUtils.dpToPx(HEIGHT_DP, context);
    widthPx = (int) ViewUtils.dpToPx(WIDTH_DP, context);
    AdapterParameters parameters = new AdapterParameters(context, null, null, adapterFactory);

    when(frameContext.bindNewStyle(any())).thenReturn(frameContextWithStyle);
    when(frameContext.getAssetProvider()).thenReturn(assetProvider);
    when(frameContext.getCurrentStyle()).thenReturn(DEFAULT_STYLE_PROVIDER);
    when(frameContextWithStyle.getAssetProvider()).thenReturn(assetProvider);
    when(frameContextWithStyle.getCurrentStyle()).thenReturn(styleProvider);
    when(styleProvider.getPadding()).thenReturn(PADDING);
    when(styleProvider.hasRoundedCorners()).thenReturn(true);
    when(styleProvider.getRoundedCorners()).thenReturn(CORNERS);
    when(frameContext.getRoundedCornerRadius(styleProvider, context))
        .thenReturn(CORNERS.getRadius());

    adapter = new KeySupplier().getAdapter(context, parameters);
  }

  @Test
  public void testCreate() {
    assertThat(adapter).isNotNull();
  }

  @Test
  public void testCreateAdapter() {
    setStyle(HEIGHT_DP, WIDTH_DP);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);
    assertThat(adapter.getModel()).isSameAs(DEFAULT_MODEL);

    assertThat(adapter.getView()).isNotNull();

    assertThat(adapter.getComputedHeightPx()).isEqualTo(heightPx);
    assertThat(adapter.getComputedWidthPx()).isEqualTo(widthPx);
    assertThat(adapter.getBaseView().getCropToPadding()).isTrue();
    verify(styleProvider).setElementStyles(context, frameContextWithStyle, adapter.getView());
  }

  @Test
  public void testCreateAdapter_roundedCorners() {
    adapter.createAdapter(DEFAULT_MODEL, frameContext);

    assertThat(adapter.getBaseView().getCornerRadius()).isWithin(.01F).of(CORNERS.getRadius());
  }

  @Test
  public void testCreateAdapter_noRoundedCorners() {
    when(styleProvider.hasRoundedCorners()).thenReturn(false);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);

    assertThat(adapter.getBaseView().getCornerRadius()).isWithin(.01F).of(0);
  }

  @Test
  public void testCreateAdapter_noDimensionsSet() {
    setStyle(null, null);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);

    assertThat(adapter.getModel()).isSameAs(DEFAULT_MODEL);

    assertThat(adapter.getView()).isNotNull();

    // Assert that width and height are set to the defaults
    assertThat(adapter.getComputedHeightPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
    assertThat(adapter.getComputedWidthPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
  }

  @Test
  public void testCreateAdapter_heightOnly() {
    setStyle(HEIGHT_DP, null);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);

    assertThat(adapter.getModel()).isEqualTo(DEFAULT_MODEL);

    assertThat(adapter.getView()).isNotNull();

    // Width defaults to MATCH_PARENT
    assertThat(adapter.getComputedHeightPx()).isEqualTo(heightPx);
    assertThat(adapter.getComputedWidthPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
  }

  @Test
  public void testCreateAdapter_widthOnly() {
    setStyle(null, WIDTH_DP);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);

    assertThat(adapter.getModel()).isEqualTo(DEFAULT_MODEL);

    assertThat(adapter.getView()).isNotNull();

    // Image defaults to a square.
    assertThat(adapter.getComputedHeightPx()).isEqualTo(widthPx);
    assertThat(adapter.getComputedWidthPx()).isEqualTo(widthPx);
  }

  @Test
  public void testCreateAdapter_noContent() {
    ImageElement model = ImageElement.getDefaultInstance();

    adapter.createAdapter(model, frameContext);

    assertThatRunnable(() -> adapter.bindModel(model, frameContext))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Unsupported or missing content");
  }

  @Test
  public void testBindModel_image() {
    StyleIdsStack styles = StyleIdsStack.newBuilder().addStyleIds("stylecat").build();
    ImageElement model =
        ImageElement.newBuilder().setImage(DEFAULT_IMAGE).setStyleReferences(styles).build();

    adapter.createAdapter(model, frameContext);
    adapter.bindModel(model, frameContext);

    imageView = adapter.getBaseView();
    assertDrawableSet(imageView);
    assertThat(imageView.getScaleType()).isEqualTo(ScaleType.CENTER_CROP);

    assertThat(adapter.getModel()).isSameAs(model);
    assertThat(adapter.getElementStyleIdsStack()).isEqualTo(styles);
  }

  @Test
  public void testBindModel_imageBinding() {
    ImageBindingRef imageBinding = ImageBindingRef.newBuilder().setBindingId("feline").build();
    ImageElement model = ImageElement.newBuilder().setImageBinding(imageBinding).build();
    when(frameContext.getImageBindingValue(imageBinding))
        .thenReturn(BindingValue.newBuilder().setImage(DEFAULT_IMAGE).build());

    adapter.createAdapter(model, frameContext);
    adapter.bindModel(model, frameContext);

    assertDrawableSet(adapter.getBaseView());
    assertThat(adapter.getModel()).isSameAs(model);
  }

  @Test
  public void testBindModel_visibilityGone() {
    String bindingRef = "foto";
    ImageBindingRef imageBindingRef = ImageBindingRef.newBuilder().setBindingId(bindingRef).build();
    ImageElement imageBindingElement =
        ImageElement.newBuilder().setImageBinding(imageBindingRef).build();
    adapter.createAdapter(
        ImageElement.newBuilder().setImage(Image.getDefaultInstance()).build(), frameContext);
    when(frameContext.getImageBindingValue(imageBindingRef))
        .thenReturn(BindingValue.newBuilder().setVisibility(Visibility.GONE).build());

    adapter.bindModel(imageBindingElement, frameContext);

    assertThat(adapter.getView().getVisibility()).isEqualTo(View.GONE);
  }

  @Test
  public void testBindModel_optionalAbsent() {
    String bindingRef = "foto";
    ImageBindingRef imageBindingRef =
        ImageBindingRef.newBuilder().setBindingId(bindingRef).setIsOptional(true).build();
    ImageElement imageBindingElement =
        ImageElement.newBuilder().setImageBinding(imageBindingRef).build();
    adapter.createAdapter(
        ImageElement.newBuilder().setImage(Image.getDefaultInstance()).build(), frameContext);
    when(frameContext.getImageBindingValue(imageBindingRef))
        .thenReturn(BindingValue.getDefaultInstance());

    adapter.bindModel(imageBindingElement, frameContext);
    assertThat(adapter.getBaseView().getDrawable()).isNull();
    assertThat(adapter.getView().getVisibility()).isEqualTo(View.GONE);
  }

  @Test
  public void testBindModel_noContentInBindingValue() {
    String bindingRef = "foto";
    ImageBindingRef imageBindingRef = ImageBindingRef.newBuilder().setBindingId(bindingRef).build();
    ImageElement imageBindingElement =
        ImageElement.newBuilder().setImageBinding(imageBindingRef).build();
    adapter.createAdapter(
        ImageElement.newBuilder().setImage(Image.getDefaultInstance()).build(), frameContext);
    when(frameContext.getImageBindingValue(imageBindingRef))
        .thenReturn(
            BindingValue.newBuilder()
                .setBindingId(bindingRef)
                .setVisibility(Visibility.VISIBLE)
                .clearImage()
                .build());

    assertThatRunnable(() -> adapter.bindModel(imageBindingElement, frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Image binding foto had no content");
  }

  @Test
  public void testBindModel_setsVisibility() {
    String bindingRef = "foto";
    ImageBindingRef imageBindingRef = ImageBindingRef.newBuilder().setBindingId(bindingRef).build();
    ImageElement imageBindingElement =
        ImageElement.newBuilder().setImageBinding(imageBindingRef).build();
    adapter.createAdapter(
        ImageElement.newBuilder().setImage(Image.getDefaultInstance()).build(), frameContext);

    // Sets visibility on bound value with content
    when(frameContext.getImageBindingValue(imageBindingRef))
        .thenReturn(
            BindingValue.newBuilder()
                .setImage(Image.getDefaultInstance())
                .setVisibility(Visibility.INVISIBLE)
                .build());
    adapter.bindModel(imageBindingElement, frameContext);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.INVISIBLE);
    adapter.unbindModel();

    // Sets visibility for inline content
    adapter.bindModel(
        ImageElement.newBuilder().setImage(Image.getDefaultInstance()).build(), frameContext);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.VISIBLE);
    adapter.unbindModel();

    // Sets visibility on GONE binding with no content
    when(frameContext.getImageBindingValue(imageBindingRef))
        .thenReturn(BindingValue.newBuilder().setVisibility(Visibility.GONE).build());
    adapter.bindModel(imageBindingElement, frameContext);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.GONE);
    adapter.unbindModel();

    // Sets visibility with VISIBLE binding
    when(frameContext.getImageBindingValue(imageBindingRef))
        .thenReturn(
            BindingValue.newBuilder()
                .setImage(Image.getDefaultInstance())
                .setVisibility(Visibility.VISIBLE)
                .build());
    adapter.bindModel(imageBindingElement, frameContext);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.VISIBLE);
  }

  @Test
  public void testBindModel_again() {
    // Bind a model, then unbind it.
    setStyle(HEIGHT_DP, WIDTH_DP);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);
    adapter.bindModel(DEFAULT_MODEL, frameContext);
    imageView = adapter.getBaseView();
    RecyclerKey key1 = adapter.getKey();
    adapter.unbindModel();

    // Bind a different model
    ImageElement model2 = ImageElement.newBuilder().setImage(Image.getDefaultInstance()).build();
    adapter.bindModel(model2, frameContext);
    verify(assetProvider)
        .getImage(eq(Image.getDefaultInstance()), consumerArgumentCaptor.capture());

    RecyclerKey key2 = adapter.getKey();
    assertThat(key1).isSameAs(key2);
    assertThat(adapter.getModel()).isSameAs(model2);
    assertThat(adapter.getView()).isNotNull();

    Drawable drawable2 = new ColorDrawable(Color.RED);
    consumerArgumentCaptor.getValue().accept(drawable2);

    ImageView imageView2 = adapter.getBaseView();

    assertThat(imageView2).isSameAs(imageView);
    assertThat(imageView2.getDrawable()).isEqualTo(drawable2);
  }

  @Test
  public void testBindModel_bindingTwiceThrowsException() {
    setStyle(HEIGHT_DP, WIDTH_DP);

    adapter.createAdapter(DEFAULT_MODEL, frameContext);
    adapter.bindModel(DEFAULT_MODEL, frameContext);

    assertThatRunnable(() -> adapter.bindModel(DEFAULT_MODEL, frameContext))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("An image loading callback exists");
  }

  @Test
  public void testBindModel_setsStylesOnlyIfBindingIsDefined() {
    // Create an adapter with a default style
    setStyle(HEIGHT_DP, WIDTH_DP);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);

    verify(styleProvider).setElementStyles(context, frameContextWithStyle, adapter.getView());

    // Styles do not change when a different model is bound
    StyleIdsStack otherStyle = StyleIdsStack.newBuilder().addStyleIds("ignored").build();
    ImageElement imageWithOtherStyle =
        ImageElement.newBuilder()
            .setStyleReferences(otherStyle)
            .setImage(Image.getDefaultInstance())
            .build();
    FrameContext frameContextWithOtherStyle = mock(FrameContext.class);
    when(frameContext.bindNewStyle(otherStyle)).thenReturn(frameContextWithOtherStyle);
    when(frameContextWithOtherStyle.getAssetProvider()).thenReturn(assetProvider);
    adapter.bindModel(imageWithOtherStyle, frameContext);
    adapter.unbindModel();

    verify(frameContextWithOtherStyle, never()).getCurrentStyle();

    // Styles do change when a model with a style binding is bound
    StyleIdsStack boundStyle =
        StyleIdsStack.newBuilder()
            .setStyleBinding(StyleBindingRef.newBuilder().setBindingId("tuna"))
            .build();
    ImageElement imageWithBoundStyle =
        ImageElement.newBuilder()
            .setStyleReferences(boundStyle)
            .setImage(Image.getDefaultInstance())
            .build();
    FrameContext frameContextWithBoundStyle = mock(FrameContext.class);
    when(frameContext.bindNewStyle(boundStyle)).thenReturn(frameContextWithBoundStyle);
    when(frameContextWithBoundStyle.getAssetProvider()).thenReturn(assetProvider);
    when(frameContextWithBoundStyle.getCurrentStyle()).thenReturn(styleProvider);
    adapter.bindModel(imageWithBoundStyle, frameContext);

    verify(styleProvider).setElementStyles(context, frameContextWithBoundStyle, adapter.getView());
  }

  @Test
  public void testUnbind() {
    setStyle(HEIGHT_DP, WIDTH_DP);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);
    adapter.bindModel(DEFAULT_MODEL, frameContext);
    adapter.unbindModel();

    assertThat(adapter.getView()).isNotNull();
    assertThat(adapter.getBaseView().getDrawable()).isNull();

    assertThat(adapter.getComputedHeightPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
    assertThat(adapter.getComputedWidthPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
  }

  @Test
  public void testUnbind_cancelsCallback() {
    setStyle(HEIGHT_DP, WIDTH_DP);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);
    adapter.bindModel(DEFAULT_MODEL, frameContext);

    imageView = adapter.getBaseView();

    adapter.unbindModel();
    verify(assetProvider).getImage(eq(DEFAULT_IMAGE), consumerArgumentCaptor.capture());
    consumerArgumentCaptor.getValue().accept(defaultDrawable);

    // The drawable was not set by the callback because the model had already been unbound.
    assertThat(imageView.getDrawable()).isEqualTo(null);
  }

  @Test
  public void testUnbind_cancelsCallback2() {
    setStyle(HEIGHT_DP, WIDTH_DP);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);
    adapter.bindModel(DEFAULT_MODEL, frameContext);

    verify(assetProvider).getImage(eq(DEFAULT_IMAGE), consumerArgumentCaptor.capture());

    adapter.unbindModel();

    // Bind a new model before the callback has completed
    Image model2 = Image.newBuilder().setTintColor(5).build();
    adapter.bindModel(ImageElement.newBuilder().setImage(model2).build(), frameContext);

    // Callback now completes
    consumerArgumentCaptor.getValue().accept(defaultDrawable);

    // But has no effect
    imageView = adapter.getBaseView();
    assertThat(imageView.getDrawable()).isEqualTo(null);

    // And now the callback for the second model completes
    Drawable drawable2 = new ColorDrawable(Color.RED);
    verify(assetProvider).getImage(eq(model2), consumerArgumentCaptor.capture());
    consumerArgumentCaptor.getValue().accept(drawable2);

    // The image has the second drawable as expected.
    assertThat(imageView.getDrawable()).isEqualTo(drawable2);
  }

  @Test
  public void testComputedDimensions_unbound() {
    assertThat(adapter.getComputedHeightPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
    assertThat(adapter.getComputedWidthPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
  }

  @Test
  public void testComputedDimensions_bound() {
    setStyle(HEIGHT_DP, WIDTH_DP);
    adapter.createAdapter(DEFAULT_MODEL, frameContext);

    assertThat(adapter.getComputedHeightPx()).isEqualTo(heightPx);
    assertThat(adapter.getComputedWidthPx()).isEqualTo(widthPx);
  }

  @Test
  public void testGetModelFromElement() {
    ImageElement model =
        ImageElement.newBuilder()
            .setStyleReferences(StyleIdsStack.newBuilder().addStyleIds("image"))
            .build();

    Element elementWithModel = Element.newBuilder().setImageElement(model).build();
    assertThat(adapter.getModelFromElement(elementWithModel)).isSameAs(model);

    Element elementWithWrongModel =
        Element.newBuilder().setCustomElement(CustomElement.getDefaultInstance()).build();
    assertThatRunnable(() -> adapter.getModelFromElement(elementWithWrongModel))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing ImageElement");

    Element emptyElement = Element.getDefaultInstance();
    assertThatRunnable(() -> adapter.getModelFromElement(emptyElement))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing ImageElement");
  }

  private void setStyle(/*@Nullable*/ Integer height, /*@Nullable*/ Integer width) {
    if (height != null) {
      when(styleProvider.hasHeight()).thenReturn(true);
      when(styleProvider.getHeight()).thenReturn(height);
    } else {
      when(styleProvider.hasHeight()).thenReturn(false);
      when(styleProvider.getHeight()).thenReturn(0);
    }
    if (width != null) {
      when(styleProvider.hasWidth()).thenReturn(true);
      when(styleProvider.getWidth()).thenReturn(width);
    } else {
      when(styleProvider.hasWidth()).thenReturn(false);
      when(styleProvider.getWidth()).thenReturn(0);
    }
  }

  private void assertDrawableSet(ImageView imageView) {
    verify(assetProvider).getImage(eq(DEFAULT_IMAGE), consumerArgumentCaptor.capture());
    consumerArgumentCaptor.getValue().accept(defaultDrawable);
    assertThat(imageView.getDrawable()).isEqualTo(defaultDrawable);
    assertThat(imageView.getScaleType()).isEqualTo(ScaleType.CENTER_CROP);
  }
}
