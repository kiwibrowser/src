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

import static com.google.android.libraries.feed.common.Validators.checkState;

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.Space;
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.GravityVertical;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;

/** An {@link ElementContainerAdapter} which manages vertical lists of elements. */
class ElementListAdapter
    extends ElementContainerAdapter<LinearLayout, ElementAdapter<? extends View, ?>, ElementList> {
  private static final String TAG = "ElementListAdapter";

  private ElementListAdapter(Context context, AdapterParameters parameters) {
    super(context, parameters, createView(context), KeySupplier.SINGLETON_KEY);
  }

  @Override
  ElementList getModelFromElement(Element baseElement) {
    // Will also throw for ElementListBindingRef; looking up bindings is not supported here.
    if (!baseElement.hasElementList()) {
      throw new IllegalArgumentException(
          String.format("Missing ElementList; has %s", baseElement.getElementsCase()));
    }
    return baseElement.getElementList();
  }

  @Override
  void onCreateAdapter(ElementList model, Element baseElement, FrameContext frameContext) {
    for (Element line : model.getElementsList()) {
      ElementAdapter<? extends View, ?> adapter;
      if (line.hasElementListBinding()) {
        adapter =
            getParameters()
                .elementAdapterFactory
                .createElementListAdapter(ElementList.getDefaultInstance(), frameContext);
        addChildAdapter(adapter);
        getBaseView().addView(adapter.getView());
        return;
      } else {
        adapter = getParameters().elementAdapterFactory.createAdapterForElement(line, frameContext);
        adapter.createAdapter(line, frameContext);
      }
      addChildAdapter(adapter);
      getBaseView().addView(adapter.getView());

      updateChildLayoutParams(adapter);
    }
    setupGravityViews();
  }

  @Override
  void onBindModel(ElementList model, Element baseElement, FrameContext frameContext) {
    checkState(
        model.getElementsCount() == childAdapters.size(),
        "Number of elements mismatch: got %s elements, had %s adapters",
        model.getElementsCount(),
        childAdapters.size());
    for (int i = 0; i < model.getElementsCount(); i++) {
      Element cellModel = model.getElements(i);
      if (cellModel.hasElementListBinding()) {
        ElementListAdapter targetAdapter = (ElementListAdapter) childAdapters.get(i);
        createAndBindChildListAdapter(
            targetAdapter, cellModel.getElementListBinding(), cellModel, frameContext);
        // TODO: If a style on the bound ElementList could cause the adapter to create
        // a wrapper view (or lose one), remove the adapter's view from the LinearLayout and
        // re-add the result of getView().
      } else {
        childAdapters.get(i).bindModel(model.getElements(i), frameContext);
      }
    }
  }

  @Override
  void bindActions(FrameContext frameContext) {
    switch (getModel().getActionsDataCase()) {
      case ACTIONS:
        actions = getModel().getActions();
        break;
      case ACTIONS_BINDING:
        actions = frameContext.getActionsFromBinding(getModel().getActionsBinding());
        break;
      default:
        super.bindActions(frameContext);
        return;
    }
    ViewUtils.setOnClickActions(actions, getView(), frameContext.getActionHandler(), frameContext);
    setHideActionsActive();
  }

  @Override
  StyleIdsStack getElementStyleIdsStack() {
    return getModel().getStyleReferences();
  }

  @Override
  void onUnbindModel() {
    ViewUtils.clearOnClickActions(getView());
    super.onUnbindModel();

    // Bound item layout is created at bind time. We can't reuse the layout in child adapters, so we
    // call release at unbind time so that the child adapter's children can be recycled.
    ElementList model = getRawModel();
    if (model != null) {
      for (int i = 0; i < model.getElementsCount(); i++) {
        Element element = model.getElements(i);
        if (element.hasElementListBinding()) {
          childAdapters.get(i).releaseAdapter();
        }
      }
    }
  }

  @Override
  public void setLayoutParams(ViewGroup.LayoutParams layoutParams) {
    super.setLayoutParams(layoutParams);
    for (ElementAdapter<? extends View, ?> adapter : childAdapters) {
      updateChildLayoutParams(adapter);
    }
  }

  /**
   * Creates spacer views on top and/or bottom to make gravity work in a GridCell. These will be
   * destroyed on releaseAdapter.
   */
  void setupGravityViews() {
    LinearLayout listView = getBaseView();
    // Based on gravity, we may need to add empty cells above or below the content that fill the
    // parent cell, to ensure that backgrounds and actions apply to the entire cell.
    View topView;
    View bottomView;
    switch (getVerticalGravity()) {
      case GRAVITY_BOTTOM:
        topView = new Space(getContext());
        listView.addView(topView, 0);
        ((LayoutParams) topView.getLayoutParams()).weight = 1.0f;
        break;
      case GRAVITY_MIDDLE:
        topView = new Space(getContext());
        listView.addView(topView, 0);
        ((LayoutParams) topView.getLayoutParams()).weight = 1.0f;
        bottomView = new Space(getContext());
        listView.addView(bottomView);
        ((LayoutParams) bottomView.getLayoutParams()).weight = 1.0f;
        break;
      case GRAVITY_TOP:
        bottomView = new Space(getContext());
        listView.addView(bottomView);
        ((LayoutParams) bottomView.getLayoutParams()).weight = 1.0f;
        break;
      default:
        // do nothing
    }
  }

  GravityVertical getVerticalGravity() {
    ElementList model = getRawModel();
    if (model != null) {
      return model.getGravityVertical();
    } else {
      return GravityVertical.GRAVITY_VERTICAL_UNSPECIFIED;
    }
  }

  private void updateChildLayoutParams(ElementAdapter<? extends View, ?> adapter) {
    ViewGroup.LayoutParams params = getBaseView().getLayoutParams();

    // First try to set the width to the dimension the child wants.
    int width = adapter.getComputedWidthPx();

    // If a child doesn't have a computed width then we update to match the list.
    if (width == ElementAdapter.DIMENSION_NOT_SET) {

      // If the list is set to WRAP_CONTENT then we will need to make sure that children are set to
      // also WRAP_CONTENT otherwise the list can't determine the actual size.
      if (params.width == ViewGroup.LayoutParams.WRAP_CONTENT) {
        width = ViewGroup.LayoutParams.WRAP_CONTENT;
      } else {
        width = ViewGroup.LayoutParams.MATCH_PARENT;
      }
    }

    int height = adapter.getComputedHeightPx();
    height =
        height == ElementAdapter.DIMENSION_NOT_SET ? ViewGroup.LayoutParams.WRAP_CONTENT : height;

    LayoutParams childParams = new LayoutParams(width, height);

    adapter.getElementStyle().setMargins(getContext(), childParams);

    switch (adapter.getHorizontalGravity()) {
      case GRAVITY_START:
        childParams.gravity = Gravity.START;
        break;
      case GRAVITY_CENTER:
        childParams.gravity = Gravity.CENTER_HORIZONTAL;
        break;
      case GRAVITY_END:
        childParams.gravity = Gravity.END;
        break;
      default:
        childParams.gravity = Gravity.START;
    }

    adapter.setLayoutParams(childParams);
  }

  @VisibleForTesting
  static LinearLayout createView(Context context) {
    LinearLayout viewGroup = new LinearLayout(context);
    viewGroup.setOrientation(LinearLayout.VERTICAL);
    viewGroup.setLayoutParams(
        new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
    return viewGroup;
  }

  static class KeySupplier extends SingletonKeySupplier<ElementListAdapter, ElementList> {
    @Override
    public String getAdapterTag() {
      return TAG;
    }

    @Override
    public ElementListAdapter getAdapter(Context context, AdapterParameters parameters) {
      return new ElementListAdapter(context, parameters);
    }
  }
}
