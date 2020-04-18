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
import static com.google.android.libraries.feed.piet.ChunkedTextElementAdapter.SINGLE_LAYER_ID;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.text.SpannableStringBuilder;
import android.text.SpannedString;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.ImageSpan;
import android.text.style.StyleSpan;
import android.view.MotionEvent;
import android.view.View;
import android.widget.TextView;
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.android.libraries.feed.piet.ChunkedTextElementAdapter.ActionsClickableSpan;
import com.google.android.libraries.feed.piet.ChunkedTextElementAdapter.ImageSpanDrawableCallback;
import com.google.android.libraries.feed.piet.ChunkedTextElementAdapterTest.ShadowTextViewWithHeight;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.search.now.ui.piet.ActionsProto.Action;
import com.google.search.now.ui.piet.ActionsProto.Actions;
import com.google.search.now.ui.piet.BindingRefsProto.ActionsBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ChunkedTextBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.ImagesProto.Image;
import com.google.search.now.ui.piet.ImagesProto.ImageSource;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.Font.FontWeight;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import com.google.search.now.ui.piet.TextProto.Chunk;
import com.google.search.now.ui.piet.TextProto.ChunkedText;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import com.google.search.now.ui.piet.TextProto.StyledImageChunk;
import com.google.search.now.ui.piet.TextProto.StyledTextChunk;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowDrawable;
import org.robolectric.shadows.ShadowTextView;

/** Tests of the {@link ChunkedTextElementAdapter}. */
@RunWith(RobolectricTestRunner.class)
@Config(shadows = ShadowTextViewWithHeight.class)
public class ChunkedTextElementAdapterTest {
  private static final StyleIdsStack CHUNK_STYLE =
      StyleIdsStack.newBuilder().addStyleIds("roasted").build();
  private static final String CHUNKY_TEXT = "Skippy";
  private static final String PROCESSED_TEXT = "smooth";
  private static final ParameterizedText PARAMETERIZED_CHUNK_TEXT =
      ParameterizedText.newBuilder().setText(CHUNKY_TEXT).build();
  private static final Chunk TEXT_CHUNK =
      Chunk.newBuilder()
          .setTextChunk(
              StyledTextChunk.newBuilder()
                  .setParameterizedText(PARAMETERIZED_CHUNK_TEXT)
                  .setStyleReferences(CHUNK_STYLE))
          .build();
  private static final ChunkedText CHUNKED_TEXT_TEXT =
      ChunkedText.newBuilder().addChunks(TEXT_CHUNK).build();
  private static final String CHUNKY_URL = "pb.com/jif";
  private static final Image IMAGE_CHUNK_IMAGE =
      Image.newBuilder().addSources(ImageSource.newBuilder().setUrl(CHUNKY_URL)).build();
  private static final Chunk IMAGE_CHUNK =
      Chunk.newBuilder()
          .setImageChunk(
              StyledImageChunk.newBuilder()
                  .setImage(IMAGE_CHUNK_IMAGE)
                  .setStyleReferences(CHUNK_STYLE))
          .build();
  private static final String BINDING_ID = "PB";
  private static final ChunkedTextBindingRef CHUNKED_TEXT_BINDING_REF =
      ChunkedTextBindingRef.newBuilder().setBindingId(BINDING_ID).build();

  private static final int STYLE_HEIGHT_DP = 6;
  private static final int STYLE_ASPECT_RATIO = 2;
  private static final int STYLE_WIDTH_DP = STYLE_HEIGHT_DP * STYLE_ASPECT_RATIO;

  private static final int IMAGE_HEIGHT_PX = 60;
  private static final int IMAGE_ASPECT_RATIO = 3;
  private static final int IMAGE_WIDTH_PX = IMAGE_HEIGHT_PX * IMAGE_ASPECT_RATIO;

  private static final int TEXT_HEIGHT = 12;

  @Mock private FrameContext frameContext;
  @Mock private StyleProvider mockStyleProvider;
  @Mock private ParameterizedTextEvaluator mockTextEvaluator;
  @Mock private AssetProvider mockAssetProvider;
  @Mock private ActionHandler mockActionHandler;

  private Drawable drawable;
  private ShadowDrawable shadowDrawable;
  private SpannableStringBuilder spannable;

  private Context context;
  private TextView textView;

  private AdapterParameters adapterParameters;

  private ChunkedTextElementAdapter adapter;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    adapterParameters = new AdapterParameters(null, null, mockTextEvaluator, null);

    drawable = new ColorDrawable(Color.GREEN);
    shadowDrawable = Shadows.shadowOf(drawable);
    shadowDrawable.setIntrinsicHeight(IMAGE_HEIGHT_PX);
    shadowDrawable.setIntrinsicWidth(IMAGE_WIDTH_PX);
    spannable = new SpannableStringBuilder();

    textView = new TextView(context);

    when(mockTextEvaluator.evaluate(
            frameContext, ParameterizedText.newBuilder().setText(CHUNKY_TEXT).build()))
        .thenReturn(PROCESSED_TEXT);
    when(frameContext.bindNewStyle(StyleIdsStack.getDefaultInstance())).thenReturn(frameContext);
    when(frameContext.bindNewStyle(CHUNK_STYLE)).thenReturn(frameContext);
    when(frameContext.makeStyleFor(CHUNK_STYLE)).thenReturn(mockStyleProvider);
    when(frameContext.makeStyleFor(StyleIdsStack.getDefaultInstance()))
        .thenReturn(StyleProvider.DEFAULT_STYLE_PROVIDER);
    when(frameContext.getAssetProvider()).thenReturn(mockAssetProvider);
    when(frameContext.getCurrentStyle()).thenReturn(StyleProvider.DEFAULT_STYLE_PROVIDER);
    when(frameContext.getActionHandler()).thenReturn(mockActionHandler);
    when(mockStyleProvider.getFont()).thenReturn(Font.getDefaultInstance());

    adapter = new ChunkedTextElementAdapter.KeySupplier().getAdapter(context, adapterParameters);
  }

  @Test
  public void testCreate() {
    assertThat(adapter).isNotNull();
  }

  @Test
  public void testBind_chunkedText() {
    TextElement chunkedTextElement =
        TextElement.newBuilder().setChunkedText(CHUNKED_TEXT_TEXT).build();

    adapter.createAdapter(chunkedTextElement, frameContext);
    adapter.bindModel(chunkedTextElement, frameContext);

    assertThat(adapter.getBaseView().getText().toString()).isEqualTo(PROCESSED_TEXT);
  }

  @Test
  public void testBind_chunkedTextBinding() {
    TextElement chunkedTextBindingElement =
        TextElement.newBuilder().setChunkedTextBinding(CHUNKED_TEXT_BINDING_REF).build();

    when(frameContext.getChunkedTextBindingValue(CHUNKED_TEXT_BINDING_REF))
        .thenReturn(BindingValue.newBuilder().setChunkedText(CHUNKED_TEXT_TEXT).build());

    adapter.createAdapter(chunkedTextBindingElement, frameContext);
    adapter.bindModel(chunkedTextBindingElement, frameContext);

    verify(frameContext).getChunkedTextBindingValue(CHUNKED_TEXT_BINDING_REF);
    assertThat(adapter.getBaseView().getText().toString()).isEqualTo(PROCESSED_TEXT);
  }

  @Test
  public void testBind_wrongContent_fails() {
    TextElement elementWithWrongContent =
        TextElement.newBuilder()
            .setParameterizedText(ParameterizedText.getDefaultInstance())
            .build();

    adapter.createAdapter(elementWithWrongContent, frameContext);

    assertThatRunnable(() -> adapter.bindModel(elementWithWrongContent, frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Unhandled type of TextElement");
  }

  @Test
  public void testBind_missingContent_fails() {
    TextElement emptyElement = TextElement.getDefaultInstance();

    assertThatRunnable(() -> adapter.bindModel(emptyElement, frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Unhandled type of TextElement");
  }

  @Test
  public void testBind_textChunk() {
    TextElement chunkedTextElement =
        TextElement.newBuilder().setChunkedText(CHUNKED_TEXT_TEXT).build();

    adapter.createAdapter(chunkedTextElement, frameContext);
    adapter.bindModel(chunkedTextElement, frameContext);

    assertThat(adapter.getBaseView().getText().toString()).isEqualTo(PROCESSED_TEXT);
  }

  @Test
  public void testBind_imageChunk() {
    TextElement chunkedImageElement =
        TextElement.newBuilder()
            .setChunkedText(ChunkedText.newBuilder().addChunks(IMAGE_CHUNK))
            .build();
    when(frameContext.getAssetProvider()).thenReturn(mockAssetProvider);

    adapter.createAdapter(chunkedImageElement, frameContext);
    adapter.bindModel(chunkedImageElement, frameContext);

    assertThat(adapter.getBaseView().getText().toString()).isEqualTo(" ");
    assertThat(((SpannedString) adapter.getBaseView().getText()).getSpans(0, 1, ImageSpan.class))
        .hasLength(1);
  }

  @Test
  public void testBind_emptyChunk_fails() {
    TextElement elementWithEmptyChunk =
        TextElement.newBuilder()
            .setChunkedText(ChunkedText.newBuilder().addChunks(Chunk.getDefaultInstance()))
            .build();

    adapter.createAdapter(elementWithEmptyChunk, frameContext);

    assertThatRunnable(() -> adapter.bindModel(elementWithEmptyChunk, frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Unhandled type of ChunkedText Chunk");
  }

  @Test
  public void testSetTextOnView_visibilityGone() {
    TextElement chunkedTextBindingElement =
        TextElement.newBuilder().setChunkedTextBinding(CHUNKED_TEXT_BINDING_REF).build();

    when(frameContext.getChunkedTextBindingValue(CHUNKED_TEXT_BINDING_REF))
        .thenReturn(BindingValue.newBuilder().setVisibility(Visibility.GONE).build());

    adapter.createAdapter(chunkedTextBindingElement, frameContext);

    adapter.setTextOnView(frameContext, chunkedTextBindingElement);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.GONE);
  }

  @Test
  public void testSetTextOnView_optionalAbsent() {
    TextElement chunkedTextBindingElement =
        TextElement.newBuilder().setChunkedTextBinding(CHUNKED_TEXT_BINDING_REF).build();
    when(frameContext.getChunkedTextBindingValue(CHUNKED_TEXT_BINDING_REF))
        .thenReturn(BindingValue.newBuilder().setChunkedText(CHUNKED_TEXT_TEXT).build());
    adapter.createAdapter(chunkedTextBindingElement, frameContext);

    ChunkedTextBindingRef optionalBindingRef =
        CHUNKED_TEXT_BINDING_REF.toBuilder().setIsOptional(true).build();
    TextElement chunkedTextBindingElementOptional =
        TextElement.newBuilder().setChunkedTextBinding(optionalBindingRef).build();
    when(frameContext.getChunkedTextBindingValue(optionalBindingRef))
        .thenReturn(BindingValue.getDefaultInstance());

    adapter.setTextOnView(frameContext, chunkedTextBindingElementOptional);
    assertThat(adapter.getBaseView().getText().toString()).isEmpty();
    assertThat(adapter.getView().getVisibility()).isEqualTo(View.GONE);
  }

  @Test
  public void testSetTextOnView_setsVisibility() {
    TextElement chunkedTextBindingElement =
        TextElement.newBuilder().setChunkedTextBinding(CHUNKED_TEXT_BINDING_REF).build();
    adapter.createAdapter(
        TextElement.newBuilder().setChunkedText(CHUNKED_TEXT_TEXT).build(), frameContext);

    // Sets visibility on bound value with content
    when(frameContext.getChunkedTextBindingValue(CHUNKED_TEXT_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setChunkedText(CHUNKED_TEXT_TEXT)
                .setVisibility(Visibility.INVISIBLE)
                .build());
    adapter.setTextOnView(frameContext, chunkedTextBindingElement);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.INVISIBLE);

    // Sets visibility for inline content
    adapter.setTextOnView(
        frameContext, TextElement.newBuilder().setChunkedText(CHUNKED_TEXT_TEXT).build());
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.VISIBLE);

    // Sets visibility on GONE binding with no content
    when(frameContext.getChunkedTextBindingValue(CHUNKED_TEXT_BINDING_REF))
        .thenReturn(BindingValue.newBuilder().setVisibility(Visibility.GONE).build());
    adapter.setTextOnView(frameContext, chunkedTextBindingElement);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.GONE);

    // Sets visibility with VISIBLE binding
    when(frameContext.getChunkedTextBindingValue(CHUNKED_TEXT_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setChunkedText(CHUNKED_TEXT_TEXT)
                .setVisibility(Visibility.VISIBLE)
                .build());
    adapter.setTextOnView(frameContext, chunkedTextBindingElement);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.VISIBLE);
  }

  @Test
  public void testSetTextOnView_noContent() {
    TextElement chunkedTextBindingElement =
        TextElement.newBuilder().setChunkedTextBinding(CHUNKED_TEXT_BINDING_REF).build();
    adapter.createAdapter(
        TextElement.newBuilder().setChunkedText(CHUNKED_TEXT_TEXT).build(), frameContext);

    when(frameContext.getChunkedTextBindingValue(CHUNKED_TEXT_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setBindingId(BINDING_ID)
                .setVisibility(Visibility.INVISIBLE)
                .build());

    assertThatRunnable(() -> adapter.setTextOnView(frameContext, chunkedTextBindingElement))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Chunked text binding PB had no content");
  }

  @Test
  public void testAddTextChunk_setsTextAfterEvaluatingParameterizedText() {
    adapter.addTextChunk(frameContext, spannable, TEXT_CHUNK);

    verify(mockTextEvaluator)
        .evaluate(frameContext, TEXT_CHUNK.getTextChunk().getParameterizedText());

    assertThat(spannable.toString()).isEqualTo(PROCESSED_TEXT);
  }

  @Test
  public void testAddTextChunk_setsStyles() {
    int color = 314159;
    Font font = Font.newBuilder().setItalic(true).setWeight(FontWeight.BOLD).setSize(271).build();
    int textSize = (int) ViewUtils.dpToPx(271, context);

    when(mockStyleProvider.hasColor()).thenReturn(true);
    when(mockStyleProvider.getColor()).thenReturn(color);
    when(mockStyleProvider.getFont()).thenReturn(font);

    adapter.addTextChunk(frameContext, spannable, TEXT_CHUNK);

    assertThat(spannable.getSpans(0, PROCESSED_TEXT.length(), Object.class)).hasLength(4);

    ForegroundColorSpan[] colorSpans =
        spannable.getSpans(0, PROCESSED_TEXT.length(), ForegroundColorSpan.class);
    assertThat(colorSpans[0].getForegroundColor()).isEqualTo(color);

    AbsoluteSizeSpan[] sizeSpans =
        spannable.getSpans(0, PROCESSED_TEXT.length(), AbsoluteSizeSpan.class);
    assertThat(sizeSpans[0].getSize()).isEqualTo(textSize);

    StyleSpan[] styleSpans = spannable.getSpans(0, PROCESSED_TEXT.length(), StyleSpan.class);
    assertThat(styleSpans[0].getStyle()).isEqualTo(Typeface.ITALIC);
    assertThat(styleSpans[1].getStyle()).isEqualTo(Typeface.BOLD);
  }

  @Test
  public void testAddImageChunk_setsImageAndDims() {
    when(mockStyleProvider.hasWidth()).thenReturn(true);
    when(mockStyleProvider.getWidth()).thenReturn(STYLE_WIDTH_DP);
    when(mockStyleProvider.hasHeight()).thenReturn(true);
    when(mockStyleProvider.getHeight()).thenReturn(STYLE_HEIGHT_DP);

    // Required to set up the local frameContext member var.
    adapter.createAdapter(TextElement.getDefaultInstance(), frameContext);

    adapter.addImageChunk(frameContext, textView, spannable, IMAGE_CHUNK);

    assertThat(spannable.toString()).isEqualTo(" ");

    ImageSpan[] imageSpans = spannable.getSpans(0, 1, ImageSpan.class);
    LayerDrawable containerDrawable = (LayerDrawable) imageSpans[0].getDrawable();

    ArgumentCaptor<ImageSpanDrawableCallback> imageCallbackCaptor =
        ArgumentCaptor.forClass(ImageSpanDrawableCallback.class);
    verify(mockAssetProvider).getImage(eq(IMAGE_CHUNK_IMAGE), imageCallbackCaptor.capture());

    // Activate the image loading callback
    Drawable imageDrawable = new ColorDrawable(123);
    imageCallbackCaptor.getValue().accept(imageDrawable);

    // Assert that we set the image on the span
    assertThat(containerDrawable.getDrawable(0)).isSameAs(imageDrawable);

    int widthPx = (int) ViewUtils.dpToPx(STYLE_WIDTH_DP, context);
    int heightPx = (int) ViewUtils.dpToPx(STYLE_HEIGHT_DP, context);
    assertThat(imageDrawable.getBounds()).isEqualTo(new Rect(0, 0, widthPx, heightPx));
  }

  @Test
  public void testBindSetsActions_inline() {
    TextElement imageChunkWithActions =
        TextElement.newBuilder()
            .setChunkedText(
                ChunkedText.newBuilder()
                    .addChunks(
                        IMAGE_CHUNK
                            .toBuilder()
                            .setActions(
                                Actions.newBuilder()
                                    .setOnClickAction(Action.getDefaultInstance()))))
            .build();
    when(frameContext.getAssetProvider()).thenReturn(mockAssetProvider);
    when(frameContext.getFrame()).thenReturn(Frame.getDefaultInstance());

    adapter.createAdapter(imageChunkWithActions, frameContext);
    adapter.bindModel(imageChunkWithActions, frameContext);

    assertThat(
            ((SpannedString) adapter.getBaseView().getText())
                .getSpans(0, 1, ActionsClickableSpan.class))
        .hasLength(1);
    MotionEvent motionEvent = MotionEvent.obtain(0, 0, MotionEvent.ACTION_UP, 0, 0, 0);
    adapter.getBaseView().dispatchTouchEvent(motionEvent);
    verify(mockActionHandler)
        .handleAction(
            Action.getDefaultInstance(), Frame.getDefaultInstance(), adapter.getBaseView(), null);
  }

  @Test
  public void testBindSetsActions_bind() {
    String bindingId = "ACTION";
    ActionsBindingRef binding = ActionsBindingRef.newBuilder().setBindingId(bindingId).build();
    TextElement imageChunkWithActions =
        TextElement.newBuilder()
            .setChunkedText(
                ChunkedText.newBuilder()
                    .addChunks(IMAGE_CHUNK.toBuilder().setActionsBinding(binding)))
            .build();
    when(frameContext.getAssetProvider()).thenReturn(mockAssetProvider);
    when(frameContext.getActionsFromBinding(binding))
        .thenReturn(Actions.newBuilder().setOnClickAction(Action.getDefaultInstance()).build());
    when(frameContext.getFrame()).thenReturn(Frame.getDefaultInstance());

    adapter.createAdapter(imageChunkWithActions, frameContext);
    adapter.bindModel(imageChunkWithActions, frameContext);

    assertThat(
            ((SpannedString) adapter.getBaseView().getText())
                .getSpans(0, 1, ActionsClickableSpan.class))
        .hasLength(1);
    MotionEvent motionEvent = MotionEvent.obtain(0, 0, MotionEvent.ACTION_UP, 0, 0, 0);
    adapter.getBaseView().dispatchTouchEvent(motionEvent);
    verify(mockActionHandler)
        .handleAction(
            Action.getDefaultInstance(), Frame.getDefaultInstance(), adapter.getBaseView(), null);
  }

  @Test
  public void testBindSetsActions_bindingNotFound() {
    String bindingId = "ACTION";
    ActionsBindingRef binding = ActionsBindingRef.newBuilder().setBindingId(bindingId).build();
    TextElement imageChunkWithActions =
        TextElement.newBuilder()
            .setChunkedText(
                ChunkedText.newBuilder()
                    .addChunks(IMAGE_CHUNK.toBuilder().setActionsBinding(binding)))
            .build();
    when(frameContext.getAssetProvider()).thenReturn(mockAssetProvider);
    when(frameContext.getActionsFromBinding(binding)).thenReturn(Actions.getDefaultInstance());
    when(frameContext.getFrame()).thenReturn(Frame.getDefaultInstance());

    adapter.createAdapter(imageChunkWithActions, frameContext);
    adapter.bindModel(imageChunkWithActions, frameContext);

    // Completes successfully, but doesn't add the clickable span.
    assertThat(
            ((SpannedString) adapter.getBaseView().getText())
                .getSpans(0, 1, ActionsClickableSpan.class))
        .isEmpty();
  }

  @Test
  public void testUnbind_cancelsCallbacks() {
    adapter.createAdapter(TextElement.getDefaultInstance(), frameContext);
    adapter.addImageChunk(frameContext, textView, spannable, IMAGE_CHUNK);

    ImageSpan[] imageSpans = spannable.getSpans(0, 1, ImageSpan.class);
    LayerDrawable containerDrawable = (LayerDrawable) imageSpans[0].getDrawable();
    ArgumentCaptor<ImageSpanDrawableCallback> imageCallbackCaptor =
        ArgumentCaptor.forClass(ImageSpanDrawableCallback.class);
    verify(mockAssetProvider).getImage(eq(IMAGE_CHUNK_IMAGE), imageCallbackCaptor.capture());

    // Unbind the model
    adapter.unbindModel();

    // Activate the image loading callback
    Drawable imageDrawable = new ColorDrawable(Color.RED);
    imageCallbackCaptor.getValue().accept(imageDrawable);

    // Assert that we did NOT set the image on the span
    assertThat(containerDrawable.getDrawable(SINGLE_LAYER_ID)).isNotSameAs(imageDrawable);
  }

  @Test
  public void testCreateKey_returnsSingleton() {
    assertThat(adapter.createKey(Font.getDefaultInstance()))
        .isSameAs(SingletonKeySupplier.SINGLETON_KEY);
  }

  @Test
  public void testSetBounds_heightAndWidth_setsBoth() {
    adapter.setBounds(
        drawable,
        new StyleProvider(
            Style.newBuilder().setHeight(STYLE_HEIGHT_DP).setWidth(STYLE_WIDTH_DP).build()),
        textView);

    int widthPx = (int) ViewUtils.dpToPx(STYLE_WIDTH_DP, context);
    int heightPx = (int) ViewUtils.dpToPx(STYLE_HEIGHT_DP, context);
    assertThat(drawable.getBounds()).isEqualTo(new Rect(0, 0, widthPx, heightPx));
  }

  @Test
  public void testSetBounds_heightOnly_aspectRatioScaled() {
    adapter.setBounds(
        drawable,
        new StyleProvider(Style.newBuilder().setHeight(STYLE_HEIGHT_DP).build()),
        textView);

    int heightPx = (int) ViewUtils.dpToPx(STYLE_HEIGHT_DP, context);
    assertThat(drawable.getBounds())
        .isEqualTo(new Rect(0, 0, heightPx * IMAGE_ASPECT_RATIO, heightPx));
  }

  @Test
  public void testSetBounds_widthOnly_aspectRatioScaled() {
    adapter.setBounds(
        drawable, new StyleProvider(Style.newBuilder().setWidth(STYLE_WIDTH_DP).build()), textView);

    int widthPx = (int) ViewUtils.dpToPx(STYLE_WIDTH_DP, context);
    assertThat(drawable.getBounds())
        .isEqualTo(new Rect(0, 0, widthPx, widthPx / IMAGE_ASPECT_RATIO));
  }

  @Test
  public void testSetBounds_noHeightOrWidth_defaultsToTextHeight() {
    adapter.setBounds(drawable, new StyleProvider(Style.getDefaultInstance()), textView);

    assertThat(drawable.getBounds())
        .isEqualTo(new Rect(0, 0, TEXT_HEIGHT * IMAGE_ASPECT_RATIO, TEXT_HEIGHT));
  }

  @Implements(TextView.class)
  public static class ShadowTextViewWithHeight extends ShadowTextView {
    @Implementation
    public int getLineHeight() {
      return TEXT_HEIGHT;
    }
  }
}
