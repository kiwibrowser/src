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
import static com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier.SINGLETON_KEY;
import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE_PROVIDER;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.text.TextUtils.TruncateAt;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;
import com.google.search.now.ui.piet.BindingRefsProto.StyleBindingRef;
import com.google.search.now.ui.piet.ElementsProto.CustomElement;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.GravityHorizontal;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.Font.FontWeight;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link TextElementAdapter}. */
@RunWith(RobolectricTestRunner.class)
public class TextElementAdapterTest {
  @Mock private FrameContext frameContext;
  @Mock private StyleProvider mockStyleProvider;

  private AdapterParameters adapterParameters;

  private Context context;

  private TextElementAdapter adapter;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    adapterParameters = new AdapterParameters(null, null, new ParameterizedTextEvaluator(), null);
    when(frameContext.getCurrentStyle()).thenReturn(StyleProvider.DEFAULT_STYLE_PROVIDER);
    when(frameContext.bindNewStyle(StyleIdsStack.getDefaultInstance())).thenReturn(frameContext);

    adapter = new TestTextElementAdapter(context, adapterParameters);
  }

  @Test
  public void testCreateAdapter_setsStyles() {
    TextElement textElement = getBaseTextElement(mockStyleProvider);
    int color = Color.RED;
    int maxLines = 72;
    when(mockStyleProvider.getFont()).thenReturn(Font.getDefaultInstance());
    when(mockStyleProvider.getColor()).thenReturn(color);
    when(mockStyleProvider.getMaxLines()).thenReturn(maxLines);

    adapter.createAdapter(textElement, frameContext);

    verify(mockStyleProvider).setElementStyles(context, frameContext, adapter.getBaseView());
    assertThat(adapter.getBaseView().getMaxLines()).isEqualTo(maxLines);
    assertThat(adapter.getBaseView().getEllipsize()).isEqualTo(TruncateAt.END);
    assertThat(adapter.getBaseView().getCurrentTextColor()).isEqualTo(color);
  }

  @Test
  public void testSetFont_boldFont() {
    Font.Builder font = Font.newBuilder();
    font.setWeight(FontWeight.BOLD);

    when(frameContext.makeStyleFor(any(StyleIdsStack.class))).thenReturn(mockStyleProvider);
    when(mockStyleProvider.getFont()).thenReturn(font.build());

    adapter.setFont(font.build());
    Typeface typeface = adapter.getBaseView().getTypeface();
    // Typeface.isBold and Typeface.isItalic don't work properly in roboelectric.
    assertThat(typeface.getStyle() & Typeface.BOLD).isGreaterThan(0);
    assertThat(typeface.getStyle() & Typeface.ITALIC).isEqualTo(0);
  }

  @Test
  public void testSetFont_italics() {
    Font.Builder font = Font.newBuilder();
    font.setItalic(true);

    when(frameContext.makeStyleFor(any(StyleIdsStack.class))).thenReturn(mockStyleProvider);
    when(mockStyleProvider.getFont()).thenReturn(font.build());

    adapter.setFont(font.build());
    Typeface typeface = adapter.getBaseView().getTypeface();
    // Typeface.isBold and Typeface.isItalic don't work properly in roboelectric.
    assertThat(typeface.getStyle() & Typeface.BOLD).isEqualTo(0);
    assertThat(typeface.getStyle() & Typeface.ITALIC).isGreaterThan(0);
  }

  @Test
  public void testBind_setsStylesOnlyIfBindingIsDefined() {
    int maxLines = 2;
    Style style = Style.newBuilder().setMaxLines(maxLines).build();
    StyleProvider styleProvider = new StyleProvider(style);
    TextElement textElement = getBaseTextElement(styleProvider);
    adapter.createAdapter(textElement, frameContext);
    adapter.bindModel(textElement, frameContext);
    assertThat(adapter.getBaseView().getMaxLines()).isEqualTo(maxLines);

    // Styles should not change on a re-bind
    adapter.unbindModel();
    StyleIdsStack otherStyle = StyleIdsStack.newBuilder().addStyleIds("ignored").build();
    textElement = getBaseTextElement().toBuilder().setStyleReferences(otherStyle).build();
    adapter.bindModel(textElement, frameContext);

    assertThat(adapter.getBaseView().getMaxLines()).isEqualTo(maxLines);
    verify(frameContext, never()).bindNewStyle(otherStyle);

    // Styles only change if new model has style bindings
    adapter.unbindModel();
    StyleIdsStack otherStyleWithBinding =
        StyleIdsStack.newBuilder()
            .setStyleBinding(StyleBindingRef.newBuilder().setBindingId("prionailurus"))
            .build();
    textElement =
        getBaseTextElement().toBuilder().setStyleReferences(otherStyleWithBinding).build();
    when(frameContext.bindNewStyle(otherStyleWithBinding)).thenReturn(frameContext);
    adapter.bindModel(textElement, frameContext);

    verify(frameContext).bindNewStyle(otherStyleWithBinding);
  }

  @Test
  public void testUnbind() {
    TextElement textElement = getBaseTextElement(null);
    adapter.createAdapter(textElement, frameContext);
    adapter.bindModel(textElement, frameContext);

    TextView adapterView = adapter.getBaseView();
    adapterView.setTextAlignment(View.TEXT_ALIGNMENT_VIEW_START);
    adapterView.setVisibility(View.GONE);
    adapterView.setText("OLD TEXT");

    adapter.unbindModel();

    assertThat(adapter.getBaseView()).isSameAs(adapterView);
    assertThat(adapterView.getTextAlignment()).isEqualTo(View.TEXT_ALIGNMENT_GRAVITY);
    assertThat(adapterView.getVisibility()).isEqualTo(View.VISIBLE);
    assertThat(adapterView.getText().toString()).isEmpty();
  }

  @Test
  public void testGetStyles() {
    StyleIdsStack elementStyles = StyleIdsStack.newBuilder().addStyleIds("hair").build();
    when(mockStyleProvider.getFont()).thenReturn(Font.getDefaultInstance());
    when(frameContext.bindNewStyle(elementStyles)).thenReturn(frameContext);
    TextElement textElement =
        getBaseTextElement(mockStyleProvider).toBuilder().setStyleReferences(elementStyles).build();

    adapter.createAdapter(textElement, frameContext);

    assertThat(adapter.getElementStyleIdsStack()).isSameAs(elementStyles);
  }

  @Test
  public void testGetModelFromElement() {
    TextElement model =
        TextElement.newBuilder()
            .setStyleReferences(StyleIdsStack.newBuilder().addStyleIds("spacer"))
            .build();

    Element elementWithModel = Element.newBuilder().setTextElement(model).build();
    assertThat(adapter.getModelFromElement(elementWithModel)).isSameAs(model);

    Element elementWithWrongModel =
        Element.newBuilder().setCustomElement(CustomElement.getDefaultInstance()).build();
    assertThatRunnable(() -> adapter.getModelFromElement(elementWithWrongModel))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing TextElement");

    Element emptyElement = Element.getDefaultInstance();
    assertThatRunnable(() -> adapter.getModelFromElement(emptyElement))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing TextElement");
  }

  @Test
  public void testHorizontalAlignment_start() {
    Element startElement =
        Element.newBuilder()
            .setGravityHorizontal(GravityHorizontal.GRAVITY_START)
            .setTextElement(getBaseTextElement())
            .build();
    adapter.createAdapter(startElement, frameContext);
    assertThat(adapter.getBaseView().getGravity()).isEqualTo(Gravity.TOP | Gravity.START);
  }

  @Test
  public void testHorizontalAlignment_center() {
    Element startElement =
        Element.newBuilder()
            .setGravityHorizontal(GravityHorizontal.GRAVITY_CENTER)
            .setTextElement(getBaseTextElement())
            .build();
    adapter.createAdapter(startElement, frameContext);
    assertThat(adapter.getBaseView().getGravity())
        .isEqualTo(Gravity.TOP | Gravity.CENTER_HORIZONTAL);
  }

  @Test
  public void testHorizontalAlignment_end() {
    Element startElement =
        Element.newBuilder()
            .setGravityHorizontal(GravityHorizontal.GRAVITY_END)
            .setTextElement(getBaseTextElement())
            .build();
    adapter.createAdapter(startElement, frameContext);
    assertThat(adapter.getBaseView().getGravity()).isEqualTo(Gravity.TOP | Gravity.END);
  }

  @Test
  public void testHorizontalAlignment_unspecified() {
    Element startElement =
        Element.newBuilder()
            .setGravityHorizontal(GravityHorizontal.GRAVITY_HORIZONTAL_UNSPECIFIED)
            .setTextElement(getBaseTextElement())
            .build();
    adapter.createAdapter(startElement, frameContext);
    assertThat(adapter.getBaseView().getGravity()).isEqualTo(Gravity.TOP | Gravity.START);
  }

  private TextElement getBaseTextElement() {
    return getBaseTextElement(null);
  }

  private TextElement getBaseTextElement(/*@Nullable*/ StyleProvider styleProvider) {
    StyleProvider sp = styleProvider != null ? styleProvider : DEFAULT_STYLE_PROVIDER;
    when(frameContext.makeStyleFor(any(StyleIdsStack.class))).thenReturn(sp);
    when(frameContext.getCurrentStyle()).thenReturn(sp);

    TextElement.Builder textElement = TextElement.newBuilder();
    return textElement.build();
  }

  private static class TestTextElementAdapter extends TextElementAdapter {
    TestTextElementAdapter(Context context, AdapterParameters parameters) {
      super(context, parameters);
    }

    @Override
    void setTextOnView(FrameContext frameContext, TextElement textElement) {
      return;
    }

    @Override
    RecyclerKey createKey(Font font) {
      return SINGLETON_KEY;
    }
  }
}
