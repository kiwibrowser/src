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
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import com.google.android.libraries.feed.piet.SpacerElementAdapter.KeySupplier;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.search.now.ui.piet.BindingRefsProto.StyleBindingRef;
import com.google.search.now.ui.piet.ElementsProto.CustomElement;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.SpacerElement;
import com.google.search.now.ui.piet.StylesProto.EdgeWidths;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link SpacerElementAdapter}. */
@RunWith(RobolectricTestRunner.class)
public class SpacerElementAdapterTest {
  @Mock private AdapterParameters adapterParameters;
  @Mock private FrameContext frameContext;
  @Mock private AssetProvider assetProvider;
  @Mock private StyleProvider mockStyleProvider;

  private Context context;

  private SpacerElementAdapter adapter;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    when(frameContext.getAssetProvider()).thenReturn(assetProvider);
    when(frameContext.bindNewStyle(StyleIdsStack.getDefaultInstance())).thenReturn(frameContext);
    when(frameContext.getCurrentStyle()).thenReturn(mockStyleProvider);
    when(mockStyleProvider.getPadding()).thenReturn(EdgeWidths.getDefaultInstance());
    adapter = new KeySupplier().getAdapter(context, adapterParameters);
  }

  @Test
  public void testCreate() {
    int paddingWidth = 2;
    SpacerElement spacerElement = SpacerElement.newBuilder().setHeight(1).build();
    EdgeWidths padding =
        EdgeWidths.newBuilder().setTop(paddingWidth).setBottom(paddingWidth).build();
    when(frameContext.makeStyleFor(any(StyleIdsStack.class))).thenReturn(mockStyleProvider);
    when(mockStyleProvider.getPadding()).thenReturn(padding);

    adapter.createAdapter(spacerElement, frameContext);
    View v = adapter.getView();
    assertThat(v).isNotNull();
    verify(mockStyleProvider).setElementStyles(context, frameContext, v);
  }

  @Test
  public void testBindAndUnbindKeepsViewButChangesParams() {
    // Bind adapter and check height
    int padding = 2;
    StyleIdsStack styleStack1 = StyleIdsStack.newBuilder().addStyleIds("1").build();
    SpacerElement spacerElement =
        SpacerElement.newBuilder().setHeight(1).setStyleReferences(styleStack1).build();
    Style style =
        Style.newBuilder()
            .setPadding(EdgeWidths.newBuilder().setTop(padding).setBottom(padding).build())
            .build();
    StyleProvider styleProvider = new StyleProvider(style);
    when(frameContext.makeStyleFor(styleStack1)).thenReturn(styleProvider);
    when(frameContext.bindNewStyle(styleStack1)).thenReturn(frameContext);

    adapter.createAdapter(spacerElement, frameContext);

    adapter.bindModel(spacerElement, frameContext);

    View v1 = adapter.getView();
    assertThat(adapter.getComputedHeightPx()).isEqualTo(5);

    // Unbind adapter
    adapter.unbindModel();

    // Bind adapter with different height and check that view is reused, but height is changes.
    // Ensure there is a StyleBindingRef so that we actually recreate the spacer.
    int padding2 = 4;
    StyleIdsStack styleStack2 =
        StyleIdsStack.newBuilder()
            .addStyleIds("2")
            .setStyleBinding(StyleBindingRef.getDefaultInstance())
            .build();
    SpacerElement spacerElement2 =
        SpacerElement.newBuilder().setHeight(2).setStyleReferences(styleStack2).build();
    Style style2 =
        Style.newBuilder()
            .setPadding(EdgeWidths.newBuilder().setTop(padding2).setBottom(padding2).build())
            .build();
    StyleProvider styleProvider2 = new StyleProvider(style2);
    when(frameContext.makeStyleFor(styleStack2)).thenReturn(styleProvider2);
    when(frameContext.bindNewStyle(styleStack2)).thenReturn(frameContext);

    adapter.bindModel(spacerElement2, frameContext);

    View v2 = adapter.getView();
    assertThat(adapter.getComputedHeightPx()).isEqualTo(10);

    assertThat(v2).isSameAs(v1);
  }

  @Test
  public void testBindOnlyChangesOnStyleBindings() {
    StyleIdsStack styles1 = StyleIdsStack.newBuilder().addStyleIds("style1").build();
    SpacerElement defaultElement =
        SpacerElement.newBuilder().setStyleReferences(styles1).setHeight(1).build();

    StyleIdsStack styles2 = StyleIdsStack.newBuilder().addStyleIds("style2").build();
    StyleProvider mockStyleProvider2 = mock(StyleProvider.class);
    SpacerElement otherElementWithStyleRefs =
        SpacerElement.newBuilder().setStyleReferences(styles2).setHeight(2).build();

    StyleIdsStack styles3 =
        StyleIdsStack.newBuilder()
            .addStyleIds("style3")
            .setStyleBinding(StyleBindingRef.newBuilder().setBindingId("styleBinding"))
            .build();
    StyleProvider mockStyleProvider3 = mock(StyleProvider.class);
    SpacerElement otherElementWithStyleBindings =
        SpacerElement.newBuilder().setStyleReferences(styles3).setHeight(3).build();

    when(frameContext.makeStyleFor(styles1)).thenReturn(mockStyleProvider);
    when(frameContext.makeStyleFor(styles2)).thenReturn(mockStyleProvider2);
    when(frameContext.makeStyleFor(styles3)).thenReturn(mockStyleProvider3);
    when(frameContext.bindNewStyle(styles1)).thenReturn(frameContext);
    when(frameContext.bindNewStyle(styles2)).thenReturn(frameContext);
    when(frameContext.bindNewStyle(styles3)).thenReturn(frameContext);
    when(mockStyleProvider.getPadding()).thenReturn(EdgeWidths.getDefaultInstance());
    when(mockStyleProvider2.getPadding()).thenReturn(EdgeWidths.getDefaultInstance());
    when(mockStyleProvider3.getPadding()).thenReturn(EdgeWidths.getDefaultInstance());

    adapter.createAdapter(defaultElement, frameContext);
    verify(frameContext).makeStyleFor(styles1);
    assertThat(adapter.getComputedHeightPx()).isEqualTo(1);

    adapter.unbindModel();

    adapter.bindModel(otherElementWithStyleRefs, frameContext);
    // Height should not change
    verify(frameContext, never()).makeStyleFor(styles2);
    assertThat(adapter.getComputedHeightPx()).isEqualTo(1);

    adapter.unbindModel();

    adapter.bindModel(otherElementWithStyleBindings, frameContext);
    verify(frameContext).makeStyleFor(styles3);
    assertThat(adapter.getComputedHeightPx()).isEqualTo(3);
  }

  @Test
  public void testGetModelFromElement() {
    SpacerElement model =
        SpacerElement.newBuilder()
            .setStyleReferences(StyleIdsStack.newBuilder().addStyleIds("spacer"))
            .build();

    Element elementWithModel = Element.newBuilder().setSpacerElement(model).build();
    assertThat(adapter.getModelFromElement(elementWithModel)).isSameAs(model);

    Element elementWithWrongModel =
        Element.newBuilder().setCustomElement(CustomElement.getDefaultInstance()).build();
    assertThatRunnable(() -> adapter.getModelFromElement(elementWithWrongModel))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing SpacerElement");

    Element emptyElement = Element.getDefaultInstance();
    assertThatRunnable(() -> adapter.getModelFromElement(emptyElement))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing SpacerElement");
  }
}
