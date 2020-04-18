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
import static com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility.GONE;
import static com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility.INVISIBLE;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.widget.TextView;
import com.google.search.now.ui.piet.BindingRefsProto.ParameterizedTextBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/**
 * Tests of the {@link ParameterizedTextElementAdapter}; also tests base features of {@link
 * TextElementAdapter}.
 */
@RunWith(RobolectricTestRunner.class)
public class ParameterizedTextElementAdapterTest {
  private static final String TEXT_LINE_CONTENT = "Content";
  private static final String BINDING = "binding";
  private static final ParameterizedTextBindingRef DEFAULT_BINDING_REF =
      ParameterizedTextBindingRef.newBuilder().setBindingId(BINDING).build();

  @Mock private FrameContext frameContext;
  @Mock private StyleProvider mockStyleProvider;

  private AdapterParameters adapterParameters;

  private Context context;

  private ParameterizedTextElementAdapter adapter;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    adapterParameters = new AdapterParameters(null, null, new ParameterizedTextEvaluator(), null);
    when(frameContext.getCurrentStyle()).thenReturn(StyleProvider.DEFAULT_STYLE_PROVIDER);
    when(frameContext.bindNewStyle(StyleIdsStack.getDefaultInstance())).thenReturn(frameContext);

    adapter =
        new ParameterizedTextElementAdapter.KeySupplier().getAdapter(context, adapterParameters);
  }

  @Test
  public void testCreate() {
    assertThat(adapter).isNotNull();
  }

  @Test
  public void testBindModel_basic() {
    TextElement textElement = getBaseTextElement();
    adapter.createAdapter(textElement, frameContext);
    adapter.bindModel(textElement, frameContext);
    assertThat(adapter.getView()).isNotNull();
    TextView textView = adapter.getBaseView();
    assertThat(textView).isNotNull();
    assertThat(textView.getText()).isEqualTo(TEXT_LINE_CONTENT);
  }

  @Test
  public void testBindModel_noContent() {
    adapter.createAdapter(getBaseTextElement(), frameContext);
    TextElement textElement = TextElement.getDefaultInstance();

    assertThatRunnable(() -> adapter.bindModel(textElement, frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("TextElement missing ParameterizedText content");
  }

  @Test
  public void testBindModel_withBinding_someText() {
    ParameterizedText parameterizedText =
        ParameterizedText.newBuilder().setText(TEXT_LINE_CONTENT).build();
    BindingValue bindingValue =
        BindingValue.newBuilder().setParameterizedText(parameterizedText).build();
    when(frameContext.getParameterizedTextBindingValue(DEFAULT_BINDING_REF))
        .thenReturn(bindingValue);

    TextElement textElement = getBindingTextElement(null);
    adapter.createAdapter(textElement, frameContext);
    adapter.bindModel(textElement, frameContext);
    assertThat(adapter.getView()).isNotNull();
    TextView textView = adapter.getBaseView();
    assertThat(textView).isNotNull();
    assertThat(textView.getText()).isEqualTo(TEXT_LINE_CONTENT);
  }

  @Test
  public void testBindModel_withBinding_noBinding() {
    when(frameContext.getParameterizedTextBindingValue(DEFAULT_BINDING_REF)).thenReturn(null);

    TextElement textElement = getBindingTextElement(null);
    adapter.createAdapter(textElement, frameContext);
    adapter.bindModel(textElement, frameContext);
    assertThat(adapter.getView()).isNotNull();
    TextView textView = adapter.getBaseView();
    assertThat(textView).isNotNull();
    assertThat(textView.getText()).isEqualTo("");
    assertThat(textView.getVisibility()).isEqualTo(View.GONE);
  }

  @Test
  public void testBindModel_withBinding_noContent() {
    when(frameContext.getParameterizedTextBindingValue(DEFAULT_BINDING_REF))
        .thenReturn(BindingValue.newBuilder().setBindingId(BINDING).build());

    TextElement textElement = getBindingTextElement(null);
    adapter.createAdapter(textElement, frameContext);

    assertThatRunnable(() -> adapter.bindModel(textElement, frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Parameterized text binding binding had no content");
  }

  @Test
  public void testBindModel_withBinding_optionalAbsent() {
    TextElement textElement = getBindingTextElement(null /* StyleProvider*/);
    adapter.createAdapter(textElement, frameContext);
    TextElement textElementOptionalBinding =
        TextElement.newBuilder()
            .setParameterizedTextBinding(DEFAULT_BINDING_REF.toBuilder().setIsOptional(true))
            .build();
    when(frameContext.getParameterizedTextBindingValue(
            textElementOptionalBinding.getParameterizedTextBinding()))
        .thenReturn(BindingValue.getDefaultInstance());

    adapter.bindModel(textElementOptionalBinding, frameContext);

    assertThat(adapter.getView()).isNotNull();
    TextView textView = adapter.getBaseView();
    assertThat(textView).isNotNull();
    assertThat(textView.getText().toString()).isEmpty();
  }

  @Test
  public void testBindModel_withBinding_visibilityGone() {
    BindingValue bindingValue = BindingValue.newBuilder().setVisibility(GONE).build();
    when(frameContext.getParameterizedTextBindingValue(DEFAULT_BINDING_REF))
        .thenReturn(bindingValue);

    TextElement textElement = getBindingTextElement(null /* StyleProvider*/);
    adapter.createAdapter(textElement, frameContext);
    adapter.bindModel(textElement, frameContext);
    assertThat(adapter.getView()).isNotNull();
    TextView textView = adapter.getBaseView();
    assertThat(textView).isNotNull();
    assertThat(textView.getVisibility()).isEqualTo(View.GONE);
  }

  @Test
  public void testBindModel_withBinding_visibilityInvisible() {
    BindingValue bindingValue =
        BindingValue.newBuilder()
            .setVisibility(INVISIBLE)
            .setParameterizedText(ParameterizedText.newBuilder().setText(TEXT_LINE_CONTENT))
            .build();
    when(frameContext.getParameterizedTextBindingValue(DEFAULT_BINDING_REF))
        .thenReturn(bindingValue);

    TextElement textElement = getBindingTextElement(null /* StyleProvider*/);
    adapter.createAdapter(textElement, frameContext);
    adapter.bindModel(textElement, frameContext);
    assertThat(adapter.getView()).isNotNull();
    TextView textView = adapter.getBaseView();
    assertThat(textView).isNotNull();
    assertThat(textView.getVisibility()).isEqualTo(View.INVISIBLE);
  }

  @Test
  public void testStyles_padding() {
    TextElement textElement =
        TextElement.newBuilder().setStyleReferences(StyleIdsStack.getDefaultInstance()).build();

    when(frameContext.makeStyleFor(any(StyleIdsStack.class))).thenReturn(mockStyleProvider);
    when(frameContext.getCurrentStyle()).thenReturn(mockStyleProvider);

    when(mockStyleProvider.getFont()).thenReturn(Font.getDefaultInstance());

    adapter.createAdapter(textElement, frameContext);
    assertThat(adapter.getView()).isNotNull();
    verify(mockStyleProvider).setElementStyles(context, frameContext, adapter.getView());
    TextView textView = adapter.getBaseView();
    assertThat(textView).isNotNull();
  }

  private TextElement getBindingTextElement(/*@Nullable*/ StyleProvider styleProvider) {
    StyleProvider sp = styleProvider != null ? styleProvider : DEFAULT_STYLE_PROVIDER;
    when(frameContext.makeStyleFor(any(StyleIdsStack.class))).thenReturn(sp);
    when(frameContext.getCurrentStyle()).thenReturn(sp);
    return TextElement.newBuilder().setParameterizedTextBinding(DEFAULT_BINDING_REF).build();
  }

  private TextElement getBaseTextElement() {
    return getBaseTextElement(null);
  }

  private TextElement getBaseTextElement(/*@Nullable*/ StyleProvider styleProvider) {
    StyleProvider sp = styleProvider != null ? styleProvider : DEFAULT_STYLE_PROVIDER;
    when(frameContext.makeStyleFor(any(StyleIdsStack.class))).thenReturn(sp);
    when(frameContext.getCurrentStyle()).thenReturn(sp);

    TextElement.Builder textElement = TextElement.newBuilder();
    ParameterizedText text = ParameterizedText.newBuilder().setText(TEXT_LINE_CONTENT).build();
    textElement.setParameterizedText(text);
    return textElement.build();
  }
}
