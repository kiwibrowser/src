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
import static org.mockito.Matchers.same;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.Space;
import com.google.android.libraries.feed.common.testing.Suppliers;
import com.google.android.libraries.feed.piet.ElementListAdapter.KeySupplier;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.testing.shadows.ExtendedShadowLinearLayout;
import com.google.search.now.ui.piet.ActionsProto.Action;
import com.google.search.now.ui.piet.ActionsProto.Actions;
import com.google.search.now.ui.piet.BindingRefsProto.ActionsBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ElementListBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.StyleBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.CustomElement;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.GravityHorizontal;
import com.google.search.now.ui.piet.ElementsProto.GravityVertical;
import com.google.search.now.ui.piet.ElementsProto.ImageElement;
import com.google.search.now.ui.piet.ElementsProto.SpacerElement;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.StylesProto.EdgeWidths;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadow.api.Shadow;

/** Tests of the {@link ElementListAdapter}. */
@RunWith(RobolectricTestRunner.class)
public class ElementListAdapterTest {

  private static final String LIST_STYLE_ID = "manycats";
  private static final StyleIdsStack LIST_STYLES =
      StyleIdsStack.newBuilder().addStyleIds(LIST_STYLE_ID).build();

  private static final Element DEFAULT_ELEMENT =
      Element.newBuilder().setSpacerElement(SpacerElement.getDefaultInstance()).build();
  private static final Element IMAGE_ELEMENT =
      Element.newBuilder().setImageElement(ImageElement.getDefaultInstance()).build();
  private static final ElementList DEFAULT_LIST =
      ElementList.newBuilder().addElements(DEFAULT_ELEMENT).build();

  private static final ElementListBindingRef LIST_BINDING_REF =
      ElementListBindingRef.newBuilder().setBindingId("shopping").build();
  private static final ElementList LIST_WITH_BOUND_LIST =
      ElementList.newBuilder()
          .addElements(Element.newBuilder().setElementListBinding(LIST_BINDING_REF))
          .build();

  @Mock private ActionHandler actionHandler;
  @Mock private FrameContext frameContext;
  @Mock private FrameContext childFrameContext;
  @Mock private FrameContext listFrameContext;
  @Mock private StyleProvider styleProvider;
  @Mock private StyleProvider childStyleProvider;
  @Mock private StyleProvider listStyleProvider;

  private Context context;
  private AdapterParameters adapterParameters;

  private ElementListAdapter adapter;

  @Before
  public void setUp() {
    initMocks(this);
    context = RuntimeEnvironment.application;

    adapterParameters = new AdapterParameters(context, Suppliers.of(null));
    adapter = new KeySupplier().getAdapter(context, adapterParameters);

    when(frameContext.bindNewStyle(StyleIdsStack.getDefaultInstance()))
        .thenReturn(childFrameContext);
    when(frameContext.bindNewStyle(LIST_STYLES)).thenReturn(listFrameContext);
    when(listFrameContext.getCurrentStyle()).thenReturn(listStyleProvider);
    when(childFrameContext.makeStyleFor(any(StyleIdsStack.class)))
        .thenReturn(DEFAULT_STYLE_PROVIDER);
    when(childFrameContext.getCurrentStyle()).thenReturn(childStyleProvider);
    when(frameContext.makeStyleFor(StyleIdsStack.getDefaultInstance()))
        .thenReturn(DEFAULT_STYLE_PROVIDER);
    when(frameContext.makeStyleFor(LIST_STYLES)).thenReturn(styleProvider);
    when(frameContext.getCurrentStyle()).thenReturn(styleProvider);
    when(styleProvider.getPadding()).thenReturn(EdgeWidths.getDefaultInstance());
    when(childStyleProvider.getPadding()).thenReturn(EdgeWidths.getDefaultInstance());
    when(frameContext.getActionHandler()).thenReturn(actionHandler);
    when(childFrameContext.getActionHandler()).thenReturn(actionHandler);
    when(listFrameContext.getActionHandler()).thenReturn(actionHandler);
  }

  @Test
  public void testOnCreateAdapter_makesList() {
    ElementList listWithStyles =
        ElementList.newBuilder()
            .setStyleReferences(LIST_STYLES)
            .addElements(DEFAULT_ELEMENT)
            .addElements(DEFAULT_ELEMENT)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithStyles, frameContext);

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(3);
    assertThat(adapter.getBaseView().getOrientation()).isEqualTo(LinearLayout.VERTICAL);
  }

  @Test
  public void testOnCreateAdapter_setsStyles() {
    ElementList listWithStyles =
        ElementList.newBuilder()
            .setStyleReferences(LIST_STYLES)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithStyles, frameContext);

    verify(listStyleProvider).setElementStyles(context, listFrameContext, adapter.getView());
  }

  @Test
  public void testOnCreateAdapter_setsMargins() {
    String marginsStyleId = "spacecat";
    StyleIdsStack marginsStyles = StyleIdsStack.newBuilder().addStyleIds(marginsStyleId).build();
    Element elementWithMargins =
        Element.newBuilder()
            .setSpacerElement(SpacerElement.newBuilder().setStyleReferences(marginsStyles))
            .build();
    ElementList listWithStyles = ElementList.newBuilder().addElements(elementWithMargins).build();
    when(frameContext.makeStyleFor(marginsStyles)).thenReturn(DEFAULT_STYLE_PROVIDER);
    when(frameContext.bindNewStyle(marginsStyles)).thenReturn(childFrameContext);
    when(childFrameContext.getCurrentStyle()).thenReturn(childStyleProvider);

    adapter.createAdapter(listWithStyles, frameContext);

    // Assert that setMargins is called on the child's layout params
    ArgumentCaptor<MarginLayoutParams> capturedLayoutParams =
        ArgumentCaptor.forClass(MarginLayoutParams.class);
    verify(childStyleProvider).setMargins(eq(context), capturedLayoutParams.capture());

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(1);
    assertThat(adapter.getBaseView().getChildAt(0).getLayoutParams())
        .isInstanceOf(MarginLayoutParams.class);
    assertThat(adapter.getBaseView().getChildAt(0).getLayoutParams())
        .isSameAs(capturedLayoutParams.getValue());
  }

  @Test
  public void testOnCreateAdapter_setsLayoutParams() {
    ElementList listWithStyles =
        ElementList.newBuilder()
            .addElements(
                DEFAULT_ELEMENT.toBuilder().setGravityHorizontal(GravityHorizontal.GRAVITY_CENTER))
            .build();

    adapter.createAdapter(listWithStyles, frameContext);

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(1);
    assertThat(adapter.getBaseView().getChildAt(0).getLayoutParams())
        .isInstanceOf(LinearLayout.LayoutParams.class);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).gravity)
        .isEqualTo(Gravity.CENTER_HORIZONTAL);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).width)
        .isEqualTo(ViewGroup.LayoutParams.MATCH_PARENT);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).height)
        .isEqualTo(0);
  }

  @Test
  public void testOnBindModel_setsActions() {
    Action action = Action.newBuilder().build();
    ElementList listWithActions =
        ElementList.newBuilder()
            .addElements(DEFAULT_ELEMENT)
            .setActions(Actions.newBuilder().setOnClickAction(action))
            .build();

    adapter.createAdapter(listWithActions, frameContext);
    adapter.bindModel(listWithActions, frameContext);
    adapter.getView().callOnClick();

    assertThat(adapter.getView().hasOnClickListeners()).isTrue();
    verify(actionHandler)
        .handleAction(same(action), any(Frame.class), eq(adapter.getView()), any(String.class));
    assertThat(adapter.actions).isSameAs(listWithActions.getActions());
  }

  @Test
  public void testOnBindModel_setsBoundActions() {
    Action action = Action.getDefaultInstance();
    ActionsBindingRef actionsBindingRef =
        ActionsBindingRef.newBuilder().setBindingId("catastrophe").build();
    Actions actions = Actions.newBuilder().setOnClickAction(action).build();
    ElementList listWithActions =
        ElementList.newBuilder()
            .addElements(DEFAULT_ELEMENT)
            .setActionsBinding(actionsBindingRef)
            .build();
    when(frameContext.getActionsFromBinding(actionsBindingRef)).thenReturn(actions);

    adapter.createAdapter(listWithActions, frameContext);
    adapter.bindModel(listWithActions, frameContext);
    adapter.getView().callOnClick();

    assertThat(adapter.getView().hasOnClickListeners()).isTrue();
    verify(actionHandler)
        .handleAction(eq(action), any(Frame.class), eq(adapter.getView()), any(String.class));
    assertThat(adapter.actions).isSameAs(actions);
  }

  @Test
  public void testOnBindModel_ElementDoesntOverrideListActions() {
    ElementList baseListElement =
        ElementList.newBuilder()
            .setActions(Actions.newBuilder().setOnClickAction(Action.newBuilder().build()))
            .build();
    Element listElementWithActions =
        Element.newBuilder()
            .setElementList(baseListElement)
            .setActions(Actions.newBuilder().setOnClickAction(Action.newBuilder().build()).build())
            .build();

    ElementListAdapter listAdapter =
        new ElementListAdapter.KeySupplier().getAdapter(context, adapterParameters);

    listAdapter.createAdapter(baseListElement, listElementWithActions, frameContext);
    listAdapter.bindModel(baseListElement, listElementWithActions, frameContext);
    listAdapter.getView().callOnClick();

    assertThat(listAdapter.getView().hasOnClickListeners()).isTrue();
    verify(actionHandler)
        .handleAction(
            same(baseListElement.getActions().getOnClickAction()),
            any(Frame.class),
            eq(listAdapter.getView()),
            any(String.class));
    assertThat(listAdapter.actions).isSameAs(baseListElement.getActions());
  }

  @Test
  public void testOnBindModel_fallsBackToElementActions() {
    ElementList baseListElement = ElementList.getDefaultInstance();
    Element listElementWithActions =
        Element.newBuilder()
            .setElementList(baseListElement)
            .setActions(Actions.newBuilder().setOnClickAction(Action.newBuilder().build()))
            .build();

    ElementListAdapter listAdapter =
        new ElementListAdapter.KeySupplier().getAdapter(context, adapterParameters);

    listAdapter.createAdapter(baseListElement, listElementWithActions, frameContext);
    listAdapter.bindModel(baseListElement, listElementWithActions, frameContext);
    listAdapter.getView().callOnClick();

    assertThat(listAdapter.getView().hasOnClickListeners()).isTrue();
    verify(actionHandler)
        .handleAction(
            same(listElementWithActions.getActions().getOnClickAction()),
            any(Frame.class),
            eq(listAdapter.getView()),
            any(String.class));
  }

  @Config(shadows = {ExtendedShadowLinearLayout.class})
  @Test
  public void testTriggerActions_triggersChildren() {
    when(frameContext.getFrame()).thenReturn(Frame.getDefaultInstance());
    Element baseElement =
        Element.newBuilder()
            .setElementList(
                ElementList.newBuilder()
                    .addElements(
                        Element.newBuilder().setElementList(ElementList.getDefaultInstance())))
            .build();
    adapter.createAdapter(baseElement, frameContext);
    adapter.bindModel(baseElement, frameContext);

    // Replace the child adapter so we can verify on it
    ElementAdapter<?, ?> mockChildAdapter = mock(ElementAdapter.class);
    adapter.childAdapters.set(0, mockChildAdapter);

    ExtendedShadowLinearLayout shadowView = Shadow.extract(adapter.getView());
    adapter.getView().setVisibility(View.VISIBLE);
    shadowView.setAttachedToWindow(true);

    View viewport = new View(context);

    adapter.triggerViewActions(viewport);

    verify(mockChildAdapter).triggerViewActions(viewport);
  }

  @Test
  public void testOnBindModel_setsStylesOnlyIfBindingIsDefined() {
    ElementList listWithStyles =
        ElementList.newBuilder()
            .setStyleReferences(LIST_STYLES)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithStyles, frameContext);
    verify(frameContext).bindNewStyle(LIST_STYLES);

    // Binding an element with a different style will not apply the new style
    StyleIdsStack otherStyles = StyleIdsStack.newBuilder().addStyleIds("bobcat").build();
    ElementList otherListWithStyles =
        ElementList.newBuilder()
            .setStyleReferences(otherStyles)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.bindModel(otherListWithStyles, frameContext);
    verify(frameContext, never()).bindNewStyle(otherStyles);

    // But binding an element with a style binding will re-apply the style
    StyleIdsStack otherStylesWithBinding =
        StyleIdsStack.newBuilder()
            .addStyleIds("bobcat")
            .setStyleBinding(StyleBindingRef.newBuilder().setBindingId("lynx"))
            .build();
    ElementList otherListWithStyleBindings =
        ElementList.newBuilder()
            .setStyleReferences(otherStylesWithBinding)
            .addElements(DEFAULT_ELEMENT)
            .build();
    when(frameContext.bindNewStyle(otherStylesWithBinding)).thenReturn(frameContext);
    when(frameContext.makeStyleFor(otherStylesWithBinding)).thenReturn(styleProvider);

    adapter.bindModel(otherListWithStyleBindings, frameContext);
    verify(frameContext).bindNewStyle(otherStylesWithBinding);
  }

  @Test
  public void testOnBindModel_failsWithIncompatibleModel() {
    ElementList listWithThreeElements =
        ElementList.newBuilder()
            .setStyleReferences(LIST_STYLES)
            .addElements(DEFAULT_ELEMENT)
            .addElements(DEFAULT_ELEMENT)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithThreeElements, frameContext);
    adapter.bindModel(listWithThreeElements, frameContext);
    adapter.unbindModel();

    ElementList listWithTwoElements =
        ElementList.newBuilder()
            .setStyleReferences(LIST_STYLES)
            .addElements(DEFAULT_ELEMENT)
            .addElements(DEFAULT_ELEMENT)
            .build();

    assertThatRunnable(() -> adapter.bindModel(listWithTwoElements, frameContext))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Number of elements mismatch");
  }

  @Test
  public void testOnBindModel_elementListBindingRecreatesAdapter() {
    ElementList listWithOneElement = ElementList.newBuilder().addElements(DEFAULT_ELEMENT).build();
    ElementList listWithTwoElements =
        ElementList.newBuilder().addElements(DEFAULT_ELEMENT).addElements(DEFAULT_ELEMENT).build();

    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(BindingValue.newBuilder().setElementList(listWithOneElement).build());
    adapter.createAdapter(LIST_WITH_BOUND_LIST, frameContext);
    // The list adapter is created but has no child views.
    assertThat(((LinearLayout) adapter.getBaseView().getChildAt(0)).getChildCount()).isEqualTo(0);

    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(BindingValue.newBuilder().setElementList(listWithTwoElements).build());
    adapter.bindModel(LIST_WITH_BOUND_LIST, frameContext);
    // The list adapter creates its one view on bind.
    assertThat(((LinearLayout) adapter.getBaseView().getChildAt(0)).getChildCount()).isEqualTo(2);

    adapter.unbindModel();
    // The list adapter has been released.
    assertThat(((LinearLayout) adapter.getBaseView().getChildAt(0)).getChildCount()).isEqualTo(0);

    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(BindingValue.newBuilder().setElementList(listWithOneElement).build());
    adapter.bindModel(LIST_WITH_BOUND_LIST, frameContext);
    // The list adapter can bind to a different model.
    assertThat(((LinearLayout) adapter.getBaseView().getChildAt(0)).getChildCount()).isEqualTo(1);
  }

  @Test
  public void testOnBindModel_bindingWithVisibilityGone() {
    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setBindingId(LIST_BINDING_REF.getBindingId())
                .setElementList(DEFAULT_LIST)
                .build());
    adapter.createAdapter(LIST_WITH_BOUND_LIST, frameContext);
    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setBindingId(LIST_BINDING_REF.getBindingId())
                .setVisibility(Visibility.GONE)
                .build());

    adapter.bindModel(LIST_WITH_BOUND_LIST, frameContext);

    assertThat(adapter.getBaseView().getChildAt(0).getVisibility()).isEqualTo(View.GONE);
  }

  @Test
  public void testOnBindModel_bindingWithNoContent() {
    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setBindingId(LIST_BINDING_REF.getBindingId())
                .setElementList(DEFAULT_LIST)
                .build());
    adapter.createAdapter(LIST_WITH_BOUND_LIST, frameContext);
    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder().setBindingId(LIST_BINDING_REF.getBindingId()).build());

    assertThatRunnable(() -> adapter.bindModel(LIST_WITH_BOUND_LIST, frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("ElementList binding shopping had no content");
  }

  @Test
  public void testOnBindModel_bindingWithOptionalAbsent() {
    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setBindingId(LIST_BINDING_REF.getBindingId())
                .setElementList(DEFAULT_LIST)
                .build());
    adapter.createAdapter(LIST_WITH_BOUND_LIST, frameContext);

    ElementListBindingRef optionalBinding =
        LIST_BINDING_REF.toBuilder().setIsOptional(true).build();
    ElementList optionalBindingList =
        ElementList.newBuilder()
            .addElements(Element.newBuilder().setElementListBinding(optionalBinding))
            .build();
    when(frameContext.getElementListBindingValue(optionalBinding))
        .thenReturn(BindingValue.newBuilder().setBindingId(optionalBinding.getBindingId()).build());

    // This should not fail.
    adapter.bindModel(optionalBindingList, frameContext);
    assertThat(adapter.getBaseView().getChildAt(0).getVisibility()).isEqualTo(View.GONE);
  }

  @Test
  public void testOnBindModel_bindingSetsVisibility() {
    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setBindingId(LIST_BINDING_REF.getBindingId())
                .setElementList(DEFAULT_LIST)
                .build());
    adapter.createAdapter(LIST_WITH_BOUND_LIST, frameContext);
    when(frameContext.getElementListBindingValue(LIST_BINDING_REF))
        .thenReturn(
            BindingValue.newBuilder()
                .setBindingId(LIST_BINDING_REF.getBindingId())
                .setVisibility(Visibility.INVISIBLE)
                .setElementList(DEFAULT_LIST)
                .build());

    adapter.bindModel(LIST_WITH_BOUND_LIST, frameContext);
    assertThat(adapter.getBaseView().getChildAt(0).getVisibility()).isEqualTo(View.INVISIBLE);
  }

  @Test
  public void testUnbindModel_unsetsActions() {
    ElementList listWithActions =
        ElementList.newBuilder()
            .addElements(DEFAULT_ELEMENT)
            .setActions(Actions.newBuilder().setOnClickAction(Action.getDefaultInstance()))
            .build();

    adapter.createAdapter(listWithActions, frameContext);
    adapter.bindModel(listWithActions, frameContext);
    View view = adapter.getView();
    adapter.unbindModel();

    assertThat(view.hasOnClickListeners()).isFalse();
  }

  @Test
  public void testReleaseAdapter() {
    ElementList listWithStyles =
        ElementList.newBuilder()
            .setStyleReferences(LIST_STYLES)
            .addElements(DEFAULT_ELEMENT)
            .addElements(DEFAULT_ELEMENT)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithStyles, frameContext);
    adapter.bindModel(listWithStyles, frameContext);

    adapter.releaseAdapter();
    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(0);
    assertThat(adapter.childAdapters).isEmpty();
  }

  // Tests creation of the vertical gravity spacers, and ensures they are destroyed appropriately.
  @Test
  public void testVerticalGravity() {
    ElementList listWithStyles =
        ElementList.newBuilder()
            .setGravityVertical(GravityVertical.GRAVITY_MIDDLE)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithStyles, frameContext);

    assertThat(adapter.getVerticalGravity()).isEqualTo(GravityVertical.GRAVITY_MIDDLE);

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(3);
    assertThat(adapter.getBaseView().getChildAt(0)).isInstanceOf(Space.class);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).weight)
        .isEqualTo(1.0f);
    assertThat(adapter.getBaseView().getChildAt(2)).isInstanceOf(Space.class);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(2).getLayoutParams()).weight)
        .isEqualTo(1.0f);

    adapter.releaseAdapter();

    listWithStyles =
        ElementList.newBuilder()
            .setGravityVertical(GravityVertical.GRAVITY_TOP)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithStyles, frameContext);

    assertThat(adapter.getVerticalGravity()).isEqualTo(GravityVertical.GRAVITY_TOP);

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(2);
    assertThat(adapter.getBaseView().getChildAt(1)).isInstanceOf(Space.class);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(1).getLayoutParams()).weight)
        .isEqualTo(1.0f);

    adapter.releaseAdapter();

    listWithStyles =
        ElementList.newBuilder()
            .setGravityVertical(GravityVertical.GRAVITY_BOTTOM)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithStyles, frameContext);

    assertThat(adapter.getVerticalGravity()).isEqualTo(GravityVertical.GRAVITY_BOTTOM);

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(2);
    assertThat(adapter.getBaseView().getChildAt(0)).isInstanceOf(Space.class);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).weight)
        .isEqualTo(1.0f);

    adapter.releaseAdapter();

    listWithStyles =
        ElementList.newBuilder().clearGravityVertical().addElements(DEFAULT_ELEMENT).build();

    adapter.createAdapter(listWithStyles, frameContext);

    assertThat(adapter.getVerticalGravity())
        .isEqualTo(GravityVertical.GRAVITY_VERTICAL_UNSPECIFIED);

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(1);
  }

  @Test
  public void testGetVerticalGravity_noModel() {
    assertThat(adapter.getVerticalGravity())
        .isEqualTo(GravityVertical.GRAVITY_VERTICAL_UNSPECIFIED);
  }

  @Test
  public void testGetStyleIdsStack() {
    ElementList listWithStyles =
        ElementList.newBuilder()
            .setStyleReferences(LIST_STYLES)
            .addElements(DEFAULT_ELEMENT)
            .build();

    adapter.createAdapter(listWithStyles, frameContext);

    assertThat(adapter.getElementStyleIdsStack()).isEqualTo(LIST_STYLES);
  }

  @Test
  public void testGetModelFromElement() {
    ElementList model =
        ElementList.newBuilder()
            .setStyleReferences(StyleIdsStack.newBuilder().addStyleIds("spacer"))
            .build();

    Element elementWithModel = Element.newBuilder().setElementList(model).build();
    assertThat(adapter.getModelFromElement(elementWithModel)).isSameAs(model);

    Element elementWithListBinding =
        Element.newBuilder()
            .setElementListBinding(ElementListBindingRef.newBuilder().setBindingId("binding"))
            .build();
    assertThatRunnable(() -> adapter.getModelFromElement(elementWithListBinding))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing ElementList");

    Element elementWithWrongModel =
        Element.newBuilder().setCustomElement(CustomElement.getDefaultInstance()).build();
    assertThatRunnable(() -> adapter.getModelFromElement(elementWithWrongModel))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing ElementList");

    Element emptyElement = Element.getDefaultInstance();
    assertThatRunnable(() -> adapter.getModelFromElement(emptyElement))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Missing ElementList");
  }

  @Test
  public void testSetLayoutParams() {
    ElementList listWithStyles =
        ElementList.newBuilder()
            .addElements(
                DEFAULT_ELEMENT.toBuilder().setGravityHorizontal(GravityHorizontal.GRAVITY_CENTER))
            .build();

    adapter.createAdapter(listWithStyles, frameContext);

    LayoutParams layoutParams =
        new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    adapter.setLayoutParams(layoutParams);

    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).gravity)
        .isEqualTo(Gravity.CENTER_HORIZONTAL);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).width)
        .isEqualTo(ViewGroup.LayoutParams.WRAP_CONTENT);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).height)
        .isEqualTo(0);
  }

  @Test
  public void testSetLayoutParams_childWidthSet() {
    int childWidth = 5;
    when(childStyleProvider.hasWidth()).thenReturn(true);
    when(childStyleProvider.getWidth()).thenReturn(childWidth);

    ElementList listWithStyles = ElementList.newBuilder().addElements(IMAGE_ELEMENT).build();

    adapter.createAdapter(listWithStyles, frameContext);

    LinearLayout.LayoutParams params =
        new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    adapter.setLayoutParams(params);

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(1);
    assertThat(adapter.getBaseView().getChildAt(0).getLayoutParams())
        .isInstanceOf(LinearLayout.LayoutParams.class);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).width)
        .isEqualTo(childWidth);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).height)
        .isEqualTo(childWidth);
  }

  @Test
  public void testSetLayoutParams_widthSetOnList() {
    ElementList listWithStyles = ElementList.newBuilder().addElements(IMAGE_ELEMENT).build();

    adapter.createAdapter(listWithStyles, frameContext);

    LinearLayout.LayoutParams params =
        new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT);
    params.weight = 1;
    adapter.setLayoutParams(params);

    assertThat(adapter.getBaseView().getChildCount()).isEqualTo(1);
    assertThat(adapter.getBaseView().getChildAt(0).getLayoutParams())
        .isInstanceOf(LinearLayout.LayoutParams.class);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).width)
        .isEqualTo(ViewGroup.LayoutParams.MATCH_PARENT);
    assertThat(((LayoutParams) adapter.getBaseView().getChildAt(0).getLayoutParams()).height)
        .isEqualTo(ViewGroup.LayoutParams.WRAP_CONTENT);
  }
}
