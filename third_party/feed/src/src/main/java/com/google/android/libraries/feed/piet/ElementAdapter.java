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

import static com.google.android.libraries.feed.common.Validators.checkNotNull;
import static com.google.android.libraries.feed.common.Validators.checkState;

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.search.now.ui.piet.ActionsProto.Actions;
import com.google.search.now.ui.piet.ActionsProto.VisibilityAction;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.GravityHorizontal;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * ElementAdapter provides a base class for the various adapters created by UiElement, and provides
 * handling for overlays and any other general UiElement features. This class enforces the life
 * cycle of the BoundViewAdapter.
 *
 * @param <V> A subclass of an Android View. This is the root level view the adapter is responsible
 *     for creating. Once created, it should have the same lifetime as the adapter.
 * @param <M> The model. This is the model type which is bound to the view by the adapter.
 */
abstract class ElementAdapter<V extends View, M> {
  private static final String TAG = "ElementAdapter";

  /**
   * Value returned by getComputed[Height/Width]Px when an explicit value for the dimension has not
   * been computed, and it should be set by the parent instead.
   */
  public static final int DIMENSION_NOT_SET = -1;

  private final Context context;
  private final AdapterParameters parameters;
  private final V view;
  /*@Nullable*/ private M model;
  private Element baseElement = Element.getDefaultInstance();
  /*@Nullable*/ private RecyclerKey key;
  private FrameContext frameContext;

  /*@Nullable*/ private FrameLayout overlayView = null;
  @VisibleForTesting final List<ElementListAdapter> overlays = new ArrayList<>();

  Actions actions = Actions.getDefaultInstance();
  /** Set of actions that are currently active / triggered so they only get called once. */
  private final Set<VisibilityAction> activeVisibilityActions = new HashSet<>();

  /**
   * Desired width of this element. {@link #DIMENSION_NOT_SET} indicates no explicit width; let the
   * parent view decide.
   */
  int widthPx = DIMENSION_NOT_SET;
  /**
   * Desired height of this element. {@link #DIMENSION_NOT_SET} indicates no explicit height; let
   * the parent view decide.
   */
  int heightPx = DIMENSION_NOT_SET;

  ElementAdapter(Context context, AdapterParameters parameters, V view) {
    this.context = context;
    this.parameters = parameters;
    this.view = view;
  }

  ElementAdapter(Context context, AdapterParameters parameters, V view, RecyclerKey key) {
    this(context, parameters, view);
    this.key = key;
  }

  /**
   * Returns the View bound to the Adapter, for external use, such as adding to a layout. This is
   * not a valid call until the first Model has been bound to the Adapter.
   */
  public View getView() {
    return overlayView != null ? overlayView : view;
  }

  /**
   * Returns the base view of the adapter; the view typed to the subclass's content. This returns a
   * different result than getView when there is a wrapper FrameLayout to support overlays.
   */
  protected V getBaseView() {
    return view;
  }

  /**
   * Sets up an adapter, but does not bind data. After this, the {@link #getView()} and {@link
   * #getKey()} can be called. This method must not be called if a Model is currently bound to the
   * Adapter; call {@link #releaseAdapter()} first. Do not override; override {@link
   * #onCreateAdapter} instead.
   */
  public void createAdapter(M model, FrameContext frameContext) {
    createAdapter(model, Element.getDefaultInstance(), frameContext);
  }

  /**
   * Sets up an adapter, but does not bind data. After this, the {@link #getView()} and {@link
   * #getKey()} can be called. This method must not be called if a Model is currently bound to the
   * Adapter; call {@link #releaseAdapter()} first. Do not override; override {@link
   * #onCreateAdapter} instead.
   */
  public void createAdapter(Element baseElement, FrameContext frameContext) {
    M model = getModelFromElement(baseElement);
    createAdapter(model, baseElement, frameContext);
  }

  /**
   * Sets up an adapter, but does not bind data. After this, the {@link #getView()} and {@link
   * #getKey()} can be called. This method must not be called if a Model is currently bound to the
   * Adapter; call {@link #releaseAdapter()} first. Do not override; override {@link
   * #onCreateAdapter} instead.
   */
  public void createAdapter(M model, Element baseElement, FrameContext frameContext) {
    this.model = model;
    this.baseElement = baseElement;
    this.frameContext = frameContext.bindNewStyle(getElementStyleIdsStack());

    onCreateAdapter(model, baseElement, frameContext);

    initializeOverlays(frameContext);

    initializeOverflow(baseElement);

    this.frameContext.getCurrentStyle().setElementStyles(getContext(), this.frameContext, view);
  }

  /** Performs child-class-specific adapter creation; to be overridden. */
  void onCreateAdapter(M model, Element baseElement, FrameContext frameContext) {}

  /**
   * Binds data from a Model to the Adapter, without changing child views or styles. Do not
   * override; override {@link #onBindModel} instead.
   */
  public void bindModel(M model, FrameContext frameContext) {
    bindModel(model, Element.getDefaultInstance(), frameContext);
  }

  /**
   * Binds data from a Model to the Adapter, without changing child views or styles. Do not
   * override; override {@link #onBindModel} instead. Binds to an Element, allowing the subclass to
   * pick out the relevant model from the oneof.
   */
  public void bindModel(Element baseElement, FrameContext frameContext) {
    bindModel(getModelFromElement(baseElement), baseElement, frameContext);
  }

  /** Bind the model and initialize overlays. Calls child {@link #onBindModel} methods. */
  void bindModel(M model, Element baseElement, FrameContext frameContext) {
    this.model = model;
    this.baseElement = baseElement;
    if (getElementStyleIdsStack().hasStyleBinding()) {
      this.frameContext = frameContext.bindNewStyle(getElementStyleIdsStack());
    }

    onBindModel(model, baseElement, frameContext);

    bindOverlays(frameContext);

    bindActions(frameContext);

    // Reapply styles if we have style bindings
    if (getElementStyleIdsStack().hasStyleBinding()) {
      this.frameContext.getCurrentStyle().setElementStyles(getContext(), this.frameContext, view);
    }
  }

  /** Extracts the relevant model from the Element oneof. Throws IllegalArgument if not found. */
  abstract M getModelFromElement(Element baseElement);

  /** Performs child-adapter-specific binding logic; to be overridden. */
  void onBindModel(M model, Element baseElement, FrameContext frameContext) {}

  private void initializeOverlays(FrameContext frameContext) {
    if (baseElement.getOverlayElementsCount() > 0) {
      FrameLayout overlayWrapper = new FrameLayout(getContext());
      overlayWrapper.addView(getBaseView());

      for (ElementList overlayList : baseElement.getOverlayElementsList()) {
        ElementListAdapter overlayAdapter =
            getParameters()
                .elementAdapterFactory
                .createElementListAdapter(overlayList, frameContext);
        overlayAdapter.createAdapter(overlayList, frameContext);
        overlays.add(overlayAdapter);
        overlayWrapper.addView(overlayAdapter.getView());

        FrameLayout.LayoutParams params =
            new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        switch (overlayAdapter.getVerticalGravity()) {
          case GRAVITY_BOTTOM:
            params.gravity = Gravity.BOTTOM;
            break;
          case GRAVITY_MIDDLE:
            params.gravity = Gravity.CENTER_VERTICAL;
            break;
          case GRAVITY_TOP:
            params.gravity = Gravity.TOP;
            break;
          default:
            params.gravity = Gravity.NO_GRAVITY;
        }
        overlayAdapter.getElementStyle().setMargins(getContext(), params);
        overlayAdapter.setLayoutParams(params);
      }
      overlayView = overlayWrapper;
    }
  }

  private void bindOverlays(FrameContext frameContext) {
    if (baseElement.getOverlayElementsCount() > 0) {
      checkState(
          baseElement.getOverlayElementsCount() == overlays.size(),
          "Overlay count mismatch: %s overlay adapters, %s overlays defined",
          overlays.size(),
          baseElement.getOverlayElementsCount());
      for (int i = 0; i < baseElement.getOverlayElementsCount(); i++) {
        overlays.get(i).bindModel(baseElement.getOverlayElements(i), frameContext);
      }
    }
  }

  void bindActions(FrameContext frameContext) {
    // Set up actions
    switch (baseElement.getActionsDataCase()) {
      case ACTIONS:
        actions = baseElement.getActions();
        ViewUtils.setOnClickActions(
            actions, getView(), frameContext.getActionHandler(), frameContext);
        break;
      case ACTIONS_BINDING:
        actions = frameContext.getActionsFromBinding(baseElement.getActionsBinding());
        ViewUtils.setOnClickActions(
            actions, getView(), frameContext.getActionHandler(), frameContext);
        break;
      default:
        actions = Actions.getDefaultInstance();
        ViewUtils.clearOnClickActions(getView());
    }
    setHideActionsActive();
  }

  /** Intended to be called in onCreate when view is offscreen; sets hide actions to active. */
  void setHideActionsActive() {
    activeVisibilityActions.clear();
    // Set all "hide" actions as active, since the view is currently off screen.
    activeVisibilityActions.addAll(actions.getOnHideActionsList());
  }

  private void initializeOverflow(Element baseElement) {
    switch (baseElement.getHorizontalOverflow()) {
      case OVERFLOW_HIDDEN:
      case OVERFLOW_UNSPECIFIED:
        // These are the default behavior
        break;
      default:
        // TODO: Use the standard Piet error codes: ERR_UNSUPPORTED_FEATURE
        Logger.w(TAG, "Unsupported overflow behavior: %s", baseElement.getHorizontalOverflow());
    }
  }

  /**
   * Gets the style of the Element's content (ex. TextElement). Only valid after model has been
   * bound.
   */
  public StyleProvider getElementStyle() {
    return frameContext.getCurrentStyle();
  }

  /**
   * Returns any styles associated with the bound Element.
   *
   * <p>Element doesn't have any styles, but most sub-elements do, so should be overridden wherever
   * possible.
   */
  StyleIdsStack getElementStyleIdsStack() {
    return StyleIdsStack.getDefaultInstance();
  }

  /**
   * Unbinds the current Model from the Adapter. The Adapter will unbind all contained Adapters
   * recursively. Do not override; override {@link #onUnbindModel} instead.
   */
  public void unbindModel() {
    onUnbindModel();
    model = null;
    baseElement = Element.getDefaultInstance();

    // Unset actions
    ViewUtils.clearOnClickActions(getView());
    if (getView() != getBaseView()) {
      ViewUtils.clearOnClickActions(getBaseView());
    }

    for (ElementListAdapter overlayAdapter : overlays) {
      overlayAdapter.unbindModel();
    }
  }

  /** Performs child-adapter-specific unbind logic; to be overridden. */
  void onUnbindModel() {}

  /**
   * Releases all child adapters to recycler pools. Do not override; override {@link
   * #onReleaseAdapter} instead.
   */
  public void releaseAdapter() {
    onReleaseAdapter();

    // Destroy overlays
    if (overlayView != null) {
      overlayView.removeAllViews();
      overlayView = null;
    }
    for (ElementListAdapter overlayAdapter : overlays) {
      getParameters().elementAdapterFactory.releaseAdapter(overlayAdapter);
    }
    overlays.clear();
  }

  /** Performs child-adapter-specific release logic; to be overridden. */
  void onReleaseAdapter() {}

  /**
   * Subclasses will call this method when they are first bound to a Model to set the Key based upon
   * the Model type.
   */
  void setKey(RecyclerKey key) {
    this.key = key;
  }

  /** Returns a {@link RecyclerKey} which represents an instance of a Model based upon a type. */
  /*@Nullable*/
  public RecyclerKey getKey() {
    // There are Adapters which don't hold on to their views.  How do we want to mark them?
    return key;
  }

  /**
   * Returns the desired width of the adapter, computed from the model at bind time, or {@link
   * #DIMENSION_NOT_SET} which implies that the parent can choose the width.
   */
  int getComputedWidthPx() {
    return widthPx;
  }

  /**
   * Returns the desired height of the adapter, computed from the model at bind time, or {@link
   * #DIMENSION_NOT_SET} which implies that the parent can choose the height.
   */
  int getComputedHeightPx() {
    return heightPx;
  }

  /** Sets the layout parameters on this adapter's view. */
  public void setLayoutParams(LayoutParams layoutParams) {
    View view = getView();
    if (view == null) {
      return;
    }
    view.setLayoutParams(layoutParams);
    V baseView = getBaseView();
    if (baseView != null && baseView != view) {
      baseView.getLayoutParams().height =
          layoutParams.height == LayoutParams.WRAP_CONTENT
              ? LayoutParams.WRAP_CONTENT
              : LayoutParams.MATCH_PARENT;
      baseView.getLayoutParams().width =
          layoutParams.width == LayoutParams.WRAP_CONTENT
              ? LayoutParams.WRAP_CONTENT
              : LayoutParams.MATCH_PARENT;
    }
  }

  AdapterParameters getParameters() {
    return parameters;
  }

  Context getContext() {
    return context;
  }

  /** Returns the {@code model}, but is only legal to call when the Adapter is bound to a model */
  M getModel() {
    return checkNotNull(model);
  }

  GravityHorizontal getHorizontalGravity() {
    return baseElement.getGravityHorizontal();
  }

  /** Returns the {@code model}, this is to support testing, it can be called at any time */
  @VisibleForTesting
  /*@Nullable*/
  M getRawModel() {
    return model;
  }

  Frame getFrame() {
    return frameContext.getFrame();
  }

  FrameContext getFrameContext() {
    return frameContext;
  }

  ParameterizedTextEvaluator getTemplatedStringEvaluator() {
    return parameters.templatedStringEvaluator;
  }

  protected void setVisibility(Visibility visibility) {
    View view = getView();
    if (view == null) {
      return;
    }
    switch (visibility) {
      case INVISIBLE:
        view.setVisibility(View.INVISIBLE);
        break;
      case GONE:
        view.setVisibility(View.GONE);
        break;
      case VISIBILITY_UNSPECIFIED:
      case VISIBLE:
        view.setVisibility(View.VISIBLE);
        break;
      default:
        throw new IllegalArgumentException(String.format("Unhandled visibility: %s", visibility));
    }
  }

  void triggerViewActions(View viewport) {
    ViewUtils.maybeTriggerViewActions(
        getView(),
        viewport,
        actions,
        frameContext.getActionHandler(),
        frameContext.getFrame(),
        activeVisibilityActions);
  }
}
