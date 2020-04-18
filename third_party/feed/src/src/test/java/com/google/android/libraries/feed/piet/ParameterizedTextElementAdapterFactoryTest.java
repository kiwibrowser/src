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
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.android.libraries.feed.piet.ParameterizedTextElementAdapter.Key;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link TextElementAdapter} instance of the {@link AdapterFactory}. */
@RunWith(RobolectricTestRunner.class)
public class ParameterizedTextElementAdapterFactoryTest {
  private static final String TEXT_LINE_CONTENT = "Content";

  @Mock private FrameContext frameContext;
  @Mock ParameterizedTextElementAdapter.KeySupplier keySupplier;
  @Mock ParameterizedTextElementAdapter adapter;
  @Mock ParameterizedTextElementAdapter adapter2;

  private AdapterParameters adapterParameters;
  private Context context;

  private AdapterFactory<ParameterizedTextElementAdapter, TextElement> textElementFactory;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    adapterParameters = new AdapterParameters(null, null, new ParameterizedTextEvaluator(), null);
    when(keySupplier.getAdapter(context, adapterParameters))
        .thenReturn(adapter)
        .thenReturn(adapter2);
  }

  @Test
  public void testKeySupplier() {
    ParameterizedTextElementAdapter.KeySupplier keySupplier =
        new ParameterizedTextElementAdapter.KeySupplier();
    assertThat(keySupplier.getAdapterTag()).isEqualTo("TextElementAdapter");
    assertThat(keySupplier.getAdapter(context, adapterParameters)).isNotNull();
    assertThat(keySupplier).isNotInstanceOf(SingletonKeySupplier.class);
    assertThat(keySupplier.getKey(frameContext, getBaseTextElement(null)))
        .isNotSameAs(SINGLETON_KEY);
    // TODO: When we finish Font support we need to test the getKey()
  }

  @Test
  public void testGetAdapterFromFactory() {
    textElementFactory = new AdapterFactory<>(context, adapterParameters, keySupplier);
    TextElement textElement = getBaseTextElement(null);

    ParameterizedTextElementAdapter textElementAdapter =
        textElementFactory.get(textElement, frameContext);

    // Verify we get the adapter from the KeySupplier, and we create but not bind it.
    assertThat(textElementAdapter).isSameAs(adapter);
    verify(adapter, never()).createAdapter(any(), any(), any());
    verify(adapter, never()).bindModel(any(), any(), any());
  }

  @Test
  public void testReleaseAndRecycling() {
    textElementFactory = new AdapterFactory<>(context, adapterParameters, keySupplier);
    TextElement textElement = getBaseTextElement(null);
    Key adapterKey = new Key(DEFAULT_STYLE_PROVIDER.getFont());
    when(adapter.getKey()).thenReturn(adapterKey);
    when(keySupplier.getKey(frameContext, textElement)).thenReturn(adapterKey);

    ParameterizedTextElementAdapter textElementAdapter =
        textElementFactory.get(textElement, frameContext);
    assertThat(textElementAdapter).isSameAs(adapter);
    textElementAdapter.createAdapter(textElement, frameContext);

    // Ensure that releasing in the factory unbinds and releases the adapter.
    textElementFactory.release(textElementAdapter);
    verify(adapter).unbindModel();
    verify(adapter).releaseAdapter();

    // Verify we get the same item when we create it again.
    TextElementAdapter textElementAdapter2 = textElementFactory.get(textElement, frameContext);
    assertThat(textElementAdapter2).isSameAs(adapter);
    assertThat(textElementAdapter2).isEqualTo(textElementAdapter);
    verify(adapter, never()).createAdapter(textElement, Element.getDefaultInstance(), frameContext);
    verify(adapter, never()).bindModel(any(), any(), any());

    // Verify we get a new item when we create another.
    TextElementAdapter textElementAdapter3 = textElementFactory.get(textElement, frameContext);
    assertThat(textElementAdapter3).isSameAs(adapter2);
    assertThat(textElementAdapter3).isNotSameAs(textElementAdapter);
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
