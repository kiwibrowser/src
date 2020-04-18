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
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.FrameLayout;
import com.google.android.libraries.feed.common.testing.Suppliers;
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.testing.shadows.ExtendedShadowView;
import com.google.search.now.ui.piet.ActionsProto.Action;
import com.google.search.now.ui.piet.ActionsProto.Actions;
import com.google.search.now.ui.piet.ActionsProto.VisibilityAction;
import com.google.search.now.ui.piet.BindingRefsProto.ActionsBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.GravityHorizontal;
import com.google.search.now.ui.piet.ElementsProto.GravityVertical;
import com.google.search.now.ui.piet.PietProto.Frame;
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

/** Tests of the {@link ElementAdapter}. */
@RunWith(RobolectricTestRunner.class)
public class ElementAdapterTest {
  private static final int WIDTH = 123;
  private static final int HEIGHT = 456;

  @Mock private FrameContext frameContext;
  @Mock private ActionHandler actionHandler;
  @Mock private StyleProvider styleProvider;

  private Context context;
  private AdapterParameters parameters;
  private View view;

  private TestElementAdapter adapter;

  @Before
  public void setUp() {
    initMocks(this);
    context = RuntimeEnvironment.application;
    parameters = new AdapterParameters(context, Suppliers.of((ViewGroup) null));
    when(frameContext.bindNewStyle(any(StyleIdsStack.class))).thenReturn(frameContext);
    when(frameContext.makeStyleFor(any(StyleIdsStack.class))).thenReturn(DEFAULT_STYLE_PROVIDER);
    when(frameContext.getCurrentStyle()).thenReturn(styleProvider);
    when(frameContext.getActionHandler()).thenReturn(actionHandler);
    view = new View(context);
    adapter = new TestElementAdapter(context, parameters, view);
  }

  @Test
  public void testGetters() {
    Element defaultElement =
        Element.newBuilder()
            .setVed("VED")
            .setGravityHorizontal(GravityHorizontal.GRAVITY_CENTER)
            .build();
    Frame frame = Frame.newBuilder().setTag("FRAME").build();
    when(frameContext.getFrame()).thenReturn(frame);

    adapter.createAdapter(defaultElement, defaultElement, frameContext);

    assertThat(adapter.getContext()).isSameAs(context);
    assertThat(adapter.getModel()).isSameAs(defaultElement);
    assertThat(adapter.getRawModel()).isSameAs(defaultElement);
    assertThat(adapter.getHorizontalGravity()).isEqualTo(GravityHorizontal.GRAVITY_CENTER);
    assertThat(adapter.getFrame()).isEqualTo(frame);
    assertThat(adapter.getFrameContext()).isSameAs(frameContext);
    assertThat(adapter.getParameters()).isSameAs(parameters);
    assertThat(adapter.getTemplatedStringEvaluator()).isSameAs(parameters.templatedStringEvaluator);
    assertThat(adapter.getElementStyle()).isSameAs(styleProvider);
    assertThat(adapter.getElementStyleIdsStack()).isEqualTo(StyleIdsStack.getDefaultInstance());
  }

  @Test
  public void testSetLayoutParams() {
    adapter.setLayoutParams(new LayoutParams(WIDTH, HEIGHT));

    assertThat(view.getLayoutParams().width).isEqualTo(WIDTH);
    assertThat(view.getLayoutParams().height).isEqualTo(HEIGHT);
  }

  @Test
  public void testSetLayoutParams_overlay() {
    Element elementWithOverlays =
        Element.newBuilder().addOverlayElements(ElementList.getDefaultInstance()).build();

    adapter.createAdapter(elementWithOverlays, elementWithOverlays, frameContext);

    adapter.setLayoutParams(new LayoutParams(WIDTH, HEIGHT));

    assertThat(adapter.getBaseView()).isNotEqualTo(adapter.getView());
    assertThat(adapter.getView().getLayoutParams().width).isEqualTo(WIDTH);
    assertThat(adapter.getView().getLayoutParams().height).isEqualTo(HEIGHT);
    assertThat(adapter.getBaseView().getLayoutParams().width).isEqualTo(LayoutParams.MATCH_PARENT);
    assertThat(adapter.getBaseView().getLayoutParams().height).isEqualTo(LayoutParams.MATCH_PARENT);

    adapter.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));

    assertThat(adapter.getBaseView()).isNotEqualTo(adapter.getView());
    assertThat(adapter.getView().getLayoutParams().width).isEqualTo(LayoutParams.WRAP_CONTENT);
    assertThat(adapter.getView().getLayoutParams().height).isEqualTo(LayoutParams.WRAP_CONTENT);
    assertThat(adapter.getBaseView().getLayoutParams().width).isEqualTo(LayoutParams.WRAP_CONTENT);
    assertThat(adapter.getBaseView().getLayoutParams().height).isEqualTo(LayoutParams.WRAP_CONTENT);
  }

  @Test
  public void testGetComputedDimensions_defaults() {
    assertThat(adapter.getComputedWidthPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
    assertThat(adapter.getComputedHeightPx()).isEqualTo(ElementAdapter.DIMENSION_NOT_SET);
  }

  @Test
  public void testGetComputedDimensions_explicit() {
    adapter.setDims(WIDTH, HEIGHT);

    assertThat(adapter.getComputedWidthPx()).isEqualTo(WIDTH);
    assertThat(adapter.getComputedHeightPx()).isEqualTo(HEIGHT);
  }

  @Test
  public void testCreateAdapter_callsOnCreateAdapter() {
    Element defaultElement = Element.getDefaultInstance();

    adapter.createAdapter(defaultElement, defaultElement, frameContext);

    assertThat(adapter.created).isTrue();
  }

  @Test
  public void testCreateAdapter_extractsModelFromElement() {
    Element element = Element.getDefaultInstance();

    adapter.createAdapter(element, frameContext);

    assertThat(adapter.getModel()).isEqualTo(TestElementAdapter.DEFAULT_MODEL);
  }

  @Test
  public void testCreateAdapter_createsOverlays() {
    Element elementWithOverlays =
        Element.newBuilder().addOverlayElements(ElementList.getDefaultInstance()).build();

    adapter.createAdapter(elementWithOverlays, elementWithOverlays, frameContext);

    assertThat(adapter.getView()).isNotEqualTo(adapter.getBaseView());
    assertThat(adapter.getView()).isInstanceOf(FrameLayout.class);
    // Check gravity on the overlay view
    assertThat(
            ((FrameLayout.LayoutParams)
                    ((FrameLayout) adapter.getView()).getChildAt(1).getLayoutParams())
                .gravity)
        .isEqualTo(Gravity.NO_GRAVITY);
  }

  @Test
  public void testCreateAdapter_createsOverlaysWithGravity() {
    Element elementWithOverlays =
        Element.newBuilder()
            .addOverlayElements(
                ElementList.newBuilder().setGravityVertical(GravityVertical.GRAVITY_MIDDLE))
            .build();

    adapter.createAdapter(elementWithOverlays, elementWithOverlays, frameContext);

    assertThat(adapter.getView()).isNotEqualTo(adapter.getBaseView());
    assertThat(adapter.getView()).isInstanceOf(FrameLayout.class);
    // Check gravity on the overlay view
    assertThat(
            ((FrameLayout.LayoutParams)
                    ((FrameLayout) adapter.getView()).getChildAt(1).getLayoutParams())
                .gravity)
        .isEqualTo(Gravity.CENTER_VERTICAL);
  }

  @Test
  public void testCreateAdapter_setsOverlayLayoutParams() {
    Element elementWithOverlays =
        Element.newBuilder().addOverlayElements(ElementList.getDefaultInstance()).build();

    ElementAdapterFactory mockFactory = mock(ElementAdapterFactory.class);
    ElementListAdapter mockOverlayAdapter = mock(ElementListAdapter.class);
    View overlayView = new View(context);
    when(mockFactory.createElementListAdapter(ElementList.getDefaultInstance(), frameContext))
        .thenReturn(mockOverlayAdapter);
    when(mockOverlayAdapter.getView()).thenReturn(overlayView);
    when(mockOverlayAdapter.getVerticalGravity()).thenReturn(GravityVertical.GRAVITY_TOP);
    when(mockOverlayAdapter.getElementStyle()).thenReturn(styleProvider);

    parameters =
        new AdapterParameters(
            context,
            Suppliers.of((ViewGroup) null),
            new ParameterizedTextEvaluator(),
            mockFactory);
    adapter = new TestElementAdapter(context, parameters, view);

    adapter.createAdapter(elementWithOverlays, elementWithOverlays, frameContext);

    assertThat((((FrameLayout) adapter.getView()).getChildAt(1))).isSameAs(overlayView);
    ArgumentCaptor<LayoutParams> layoutParamsCaptor = ArgumentCaptor.forClass(LayoutParams.class);
    verify(mockOverlayAdapter).setLayoutParams(layoutParamsCaptor.capture());
    assertThat(layoutParamsCaptor.getValue().width).isEqualTo(LayoutParams.MATCH_PARENT);
    assertThat(layoutParamsCaptor.getValue().height).isEqualTo(LayoutParams.WRAP_CONTENT);
    verify(styleProvider).setMargins(context, (MarginLayoutParams) layoutParamsCaptor.getValue());
  }

  @Test
  public void testBindModel_callsOnBindModel() {
    Element defaultElement = Element.getDefaultInstance();

    adapter.bindModel(defaultElement, defaultElement, frameContext);

    assertThat(adapter.bound).isTrue();
  }

  @Test
  public void testBindModel_extractsModelFromElement() {
    Element element = Element.getDefaultInstance();

    adapter.bindModel(element, frameContext);

    assertThat(adapter.getModel()).isEqualTo(TestElementAdapter.DEFAULT_MODEL);
  }

  @Test
  public void testBindModel_bindsOverlays() {
    ElementList overlayModel =
        ElementList.newBuilder().setGravityVertical(GravityVertical.GRAVITY_MIDDLE).build();
    Element elementWithOverlays = Element.newBuilder().addOverlayElements(overlayModel).build();

    adapter.createAdapter(elementWithOverlays, elementWithOverlays, frameContext);
    assertThat(adapter.overlays.get(0)).isInstanceOf(ElementListAdapter.class);

    ElementListAdapter mockOverlayAdapter = mock(ElementListAdapter.class);
    adapter.overlays.set(0, mockOverlayAdapter);

    adapter.bindModel(elementWithOverlays, elementWithOverlays, frameContext);
    verify(mockOverlayAdapter).bindModel(overlayModel, frameContext);
  }

  @Test
  public void testBindModel_setsActions() {
    Actions actions = Actions.newBuilder().setOnClickAction(Action.getDefaultInstance()).build();
    Element elementWithActions = Element.newBuilder().setActions(actions).build();

    adapter.bindModel(elementWithActions, elementWithActions, frameContext);
    assertThat(adapter.getView().hasOnClickListeners()).isTrue();
    assertThat(adapter.actions).isSameAs(actions);
  }

  @Test
  public void testBindModel_setsActionsWithBinding() {
    Actions actions = Actions.newBuilder().setOnClickAction(Action.getDefaultInstance()).build();
    ActionsBindingRef actionsBinding =
        ActionsBindingRef.newBuilder().setBindingId("ACTION!").build();
    Element elementWithActions = Element.newBuilder().setActionsBinding(actionsBinding).build();
    when(frameContext.getActionsFromBinding(actionsBinding)).thenReturn(actions);

    adapter.bindModel(elementWithActions, elementWithActions, frameContext);
    assertThat(adapter.getView().hasOnClickListeners()).isTrue();
    assertThat(adapter.actions).isSameAs(actions);
  }

  @Test
  public void testBindModel_unsetsActions() {
    Actions actions = Actions.newBuilder().setOnClickAction(Action.getDefaultInstance()).build();
    Element elementWithActions = Element.newBuilder().setActions(actions).build();
    Element elementWithoutActions = Element.getDefaultInstance();

    adapter.bindModel(elementWithActions, elementWithActions, frameContext);
    assertThat(adapter.getView().hasOnClickListeners()).isTrue();
    assertThat(adapter.actions).isSameAs(actions);

    adapter.bindModel(elementWithoutActions, elementWithoutActions, frameContext);
    assertThat(adapter.getView().hasOnClickListeners()).isFalse();
    assertThat(adapter.actions).isSameAs(Actions.getDefaultInstance());
  }

  @Test
  public void testBindModel_failsWithIncompatibleOverlays() {
    ElementList overlayModel =
        ElementList.newBuilder().setGravityVertical(GravityVertical.GRAVITY_MIDDLE).build();
    Element elementWithOverlays =
        Element.newBuilder()
            .addOverlayElements(overlayModel)
            .addOverlayElements(overlayModel)
            .build();
    adapter.createAdapter(elementWithOverlays, elementWithOverlays, frameContext);

    Element elementWithFewerOverlays =
        Element.newBuilder().addOverlayElements(overlayModel).build();

    assertThatRunnable(
            () ->
                adapter.bindModel(elementWithFewerOverlays, elementWithFewerOverlays, frameContext))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Overlay count mismatch");
  }

  @Test
  public void testUnbindModel_callsOnUnbindModel() {
    Element defaultElement = Element.getDefaultInstance();

    adapter.createAdapter(defaultElement, defaultElement, frameContext);
    adapter.bindModel(defaultElement, defaultElement, frameContext);
    assertThat(adapter.bound).isTrue();

    adapter.unbindModel();
    assertThat(adapter.bound).isFalse();
  }

  @Test
  public void testUnbindModel_unsetsModel() {
    Element element = Element.getDefaultInstance();

    adapter.createAdapter(element, element, frameContext);
    adapter.bindModel(element, element, frameContext);
    assertThat(adapter.getModel()).isEqualTo(element);

    adapter.unbindModel();
    assertThat(adapter.getRawModel()).isNull();
  }

  @Test
  public void testUnbindModel_unsetsActions() {
    Element elementWithOverlaysAndActions =
        Element.newBuilder()
            .addOverlayElements(
                ElementList.newBuilder()
                    .setActions(Actions.newBuilder().setOnClickAction(Action.getDefaultInstance())))
            .setActions(Actions.newBuilder().setOnClickAction(Action.getDefaultInstance()))
            .build();

    adapter.createAdapter(
        elementWithOverlaysAndActions, elementWithOverlaysAndActions, frameContext);
    adapter.bindModel(elementWithOverlaysAndActions, elementWithOverlaysAndActions, frameContext);
    View adapterView = adapter.getBaseView();
    View overlayView = adapter.getView();

    adapter.unbindModel();

    assertThat(adapterView.hasOnClickListeners()).isFalse();
    assertThat(overlayView.hasOnClickListeners()).isFalse();
  }

  @Test
  public void testUnbindModel_unbindsOverlayAdapters() {
    ElementList overlayModel =
        ElementList.newBuilder().setGravityVertical(GravityVertical.GRAVITY_MIDDLE).build();
    Element elementWithOverlays = Element.newBuilder().addOverlayElements(overlayModel).build();
    adapter.createAdapter(elementWithOverlays, elementWithOverlays, frameContext);
    assertThat(adapter.overlays.get(0)).isInstanceOf(ElementListAdapter.class);
    ElementListAdapter mockOverlayAdapter = mock(ElementListAdapter.class);
    adapter.overlays.set(0, mockOverlayAdapter);
    adapter.bindModel(elementWithOverlays, elementWithOverlays, frameContext);

    adapter.unbindModel();

    verify(mockOverlayAdapter).unbindModel();
  }

  @Test
  public void testReleaseAdapter_callsOnReleaseAdapter() {
    Element defaultElement = Element.getDefaultInstance();

    adapter.createAdapter(defaultElement, defaultElement, frameContext);
    assertThat(adapter.created).isTrue();

    adapter.releaseAdapter();
    assertThat(adapter.created).isFalse();
  }

  @Test
  public void testReleaseAdapter_removesOverlays() {
    Element elementWithOverlays =
        Element.newBuilder().addOverlayElements(ElementList.getDefaultInstance()).build();

    adapter.createAdapter(elementWithOverlays, elementWithOverlays, frameContext);
    adapter.bindModel(elementWithOverlays, elementWithOverlays, frameContext);
    View adapterView = adapter.getBaseView();
    FrameLayout overlayView = (FrameLayout) adapter.getView();

    ElementListAdapter mockOverlayAdapter = mock(ElementListAdapter.class);
    when(mockOverlayAdapter.getKey()).thenReturn(SingletonKeySupplier.SINGLETON_KEY);
    adapter.overlays.set(0, mockOverlayAdapter);

    adapter.releaseAdapter();

    assertThat(adapter.getView()).isSameAs(adapterView);
    assertThat(adapter.getView()).isSameAs(adapter.getBaseView());
    assertThat(overlayView.getChildCount()).isEqualTo(0);
    verify(mockOverlayAdapter).releaseAdapter();
  }

  @Test
  public void testSetVisibility() {
    adapter.createAdapter(Element.getDefaultInstance(), Element.getDefaultInstance(), frameContext);
    adapter.getBaseView().setVisibility(View.GONE);
    adapter.setVisibility(Visibility.VISIBLE);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.VISIBLE);
    adapter.setVisibility(Visibility.INVISIBLE);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.INVISIBLE);
    adapter.setVisibility(Visibility.GONE);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.GONE);
    adapter.setVisibility(Visibility.VISIBILITY_UNSPECIFIED);
    assertThat(adapter.getBaseView().getVisibility()).isEqualTo(View.VISIBLE);
  }

  @Config(shadows = {ExtendedShadowView.class})
  @Test
  public void testGetOnFullViewActions() {
    Frame frame = Frame.newBuilder().setTag("FRAME").build();
    when(frameContext.getFrame()).thenReturn(frame);
    View viewport = new View(context);

    // No actions defined
    adapter.createAdapter(Element.getDefaultInstance(), frameContext);
    adapter.bindModel(Element.getDefaultInstance(), frameContext);
    adapter.triggerViewActions(viewport);
    verifyZeroInteractions(actionHandler);
    adapter.releaseAdapter();

    // Actions defined, but not fullview actions
    Element elementWithNoViewActions =
        Element.newBuilder()
            .setActions(Actions.newBuilder().setOnClickAction(Action.newBuilder().build()))
            .build();
    adapter.createAdapter(elementWithNoViewActions, frameContext);
    adapter.bindModel(elementWithNoViewActions, frameContext);
    adapter.triggerViewActions(viewport);
    verifyZeroInteractions(actionHandler);
    adapter.unbindModel();
    adapter.releaseAdapter();

    // Actions defined, but not fully visible
    ExtendedShadowView viewShadow = Shadow.extract(adapter.getView());
    viewShadow.setAttachedToWindow(false);
    Actions fullViewActions =
        Actions.newBuilder()
            .addOnViewActions(VisibilityAction.newBuilder().setAction(Action.newBuilder().build()))
            .build();
    Element elementWithActions = Element.newBuilder().setActions(fullViewActions).build();
    adapter.createAdapter(elementWithActions, frameContext);
    adapter.bindModel(elementWithActions, frameContext);
    adapter.triggerViewActions(viewport);
    verifyZeroInteractions(actionHandler);
    adapter.unbindModel();
    adapter.releaseAdapter();

    // Actions defined, and fully visible
    ExtendedShadowView viewportShadow = Shadow.extract(viewport);
    viewportShadow.setLocationInWindow(0, 0);
    viewportShadow.setHeight(100);
    viewportShadow.setWidth(100);
    viewShadow.setLocationInWindow(10, 10);
    viewShadow.setHeight(50);
    viewShadow.setWidth(50);
    viewShadow.setAttachedToWindow(true);

    adapter.createAdapter(elementWithActions, frameContext);
    adapter.bindModel(elementWithActions, frameContext);
    adapter.triggerViewActions(viewport);
    verify(actionHandler)
        .handleAction(
            same(fullViewActions.getOnViewActions(0).getAction()), eq(frame), eq(view), eq(null));
  }

  @Config(shadows = {ExtendedShadowView.class})
  @Test
  public void testGetOnPartialViewActions() {
    Frame frame = Frame.newBuilder().setTag("FRAME").build();
    when(frameContext.getFrame()).thenReturn(frame);
    View viewport = new View(context);

    // No actions defined
    adapter.createAdapter(Element.getDefaultInstance(), frameContext);
    adapter.bindModel(Element.getDefaultInstance(), frameContext);
    adapter.triggerViewActions(viewport);
    verifyZeroInteractions(actionHandler);
    adapter.releaseAdapter();

    // Actions defined, but not partial view actions
    Element elementWithNoViewActions =
        Element.newBuilder()
            .setActions(Actions.newBuilder().setOnClickAction(Action.newBuilder().build()))
            .build();
    adapter.createAdapter(elementWithNoViewActions, frameContext);
    adapter.bindModel(elementWithNoViewActions, frameContext);
    adapter.triggerViewActions(viewport);
    verifyZeroInteractions(actionHandler);
    adapter.unbindModel();
    adapter.releaseAdapter();

    // Actions defined, but not attached to window
    ExtendedShadowView viewShadow = Shadow.extract(adapter.getView());
    viewShadow.setAttachedToWindow(false);
    Actions partialViewActions =
        Actions.newBuilder()
            .addOnViewActions(
                VisibilityAction.newBuilder()
                    .setAction(Action.newBuilder().build())
                    .setProportionVisible(0.01f))
            .build();
    Element elementWithActions = Element.newBuilder().setActions(partialViewActions).build();
    adapter.createAdapter(elementWithActions, frameContext);
    adapter.bindModel(elementWithActions, frameContext);
    adapter.triggerViewActions(viewport);
    verifyZeroInteractions(actionHandler);
    adapter.unbindModel();
    adapter.releaseAdapter();

    // Actions defined, and partially visible
    ExtendedShadowView viewportShadow = Shadow.extract(viewport);
    viewportShadow.setLocationInWindow(0, 0);
    viewportShadow.setHeight(100);
    viewportShadow.setWidth(100);
    viewShadow.setLocationInWindow(10, 10);
    viewShadow.setHeight(200);
    viewShadow.setWidth(200);
    viewShadow.setAttachedToWindow(true);

    adapter.createAdapter(elementWithActions, frameContext);
    adapter.bindModel(elementWithActions, frameContext);
    adapter.triggerViewActions(viewport);
    verify(actionHandler)
        .handleAction(
            same(partialViewActions.getOnViewActions(0).getAction()),
            eq(frame),
            eq(view),
            eq(null));

    // Actions defined, and fully visible
    viewShadow.setHeight(50);
    viewShadow.setWidth(50);

    adapter.createAdapter(elementWithActions, frameContext);
    adapter.bindModel(elementWithActions, frameContext);
    adapter.triggerViewActions(viewport);
    verify(actionHandler, times(2))
        .handleAction(
            same(partialViewActions.getOnViewActions(0).getAction()),
            eq(frame),
            eq(view),
            eq(null));
  }

  // Dummy implementation
  static class TestElementAdapter extends ElementAdapter<View, Object> {
    static final String DEFAULT_MODEL = "MODEL";

    boolean created = false;
    boolean bound = false;

    View adapterView;

    TestElementAdapter(Context context, AdapterParameters parameters, View adapterView) {
      super(context, parameters, adapterView);
      this.adapterView = adapterView;
    }

    @Override
    protected Object getModelFromElement(Element baseElement) {
      return DEFAULT_MODEL;
    }

    @Override
    protected void onCreateAdapter(Object model, Element baseElement, FrameContext frameContext) {
      created = true;
    }

    @Override
    protected void onBindModel(Object model, Element baseElement, FrameContext frameContext) {
      bound = true;
    }

    @Override
    protected void onUnbindModel() {
      bound = false;
    }

    @Override
    protected void onReleaseAdapter() {
      created = false;
    }

    public void setDims(int width, int height) {
      widthPx = width;
      heightPx = height;
    }
  }
}
