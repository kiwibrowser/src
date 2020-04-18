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

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.view.View;
import android.view.ViewGroup;
import com.google.search.now.ui.piet.BindingRefsProto.ElementListBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import java.util.ArrayList;
import java.util.List;

/**
 * Base class for adapters that act as containers for other adapters, such as ElementList and
 * GridRow. Ensures that lifecycle methods are called on child adapters when the parent class binds,
 * unbinds, or releases.
 */
abstract class ElementContainerAdapter<
        V extends ViewGroup, ChildAdapterT extends ElementAdapter<? extends View, ?>, M>
    extends ElementAdapter<V, M> {

  @VisibleForTesting final List<ChildAdapterT> childAdapters;

  ElementContainerAdapter(Context context, AdapterParameters parameters, V view, RecyclerKey key) {
    super(context, parameters, view, key);
    childAdapters = new ArrayList<>();
  }

  ElementContainerAdapter(Context context, AdapterParameters parameters, V view) {
    super(context, parameters, view);
    childAdapters = new ArrayList<>();
  }

  /** Unbind the model and release child adapters. Be sure to call this in any overrides. */
  @Override
  void onUnbindModel() {
    for (ElementAdapter<?, ?> childAdapter : childAdapters) {
      childAdapter.unbindModel();
    }
    super.onUnbindModel();
  }

  @Override
  void onReleaseAdapter() {
    V containerView = getBaseView();
    if (containerView != null) {
      containerView.removeAllViews();
    }
    for (ElementAdapter<?, ?> childAdapter : childAdapters) {
      getParameters().elementAdapterFactory.releaseAdapter(childAdapter);
    }
    childAdapters.clear();
  }

  /**
   * When we bind an item that has an ElementListBinding, the adapter hasn't been initialized yet so
   * we need to both create and bind it.
   */
  void createAndBindChildListAdapter(
      ElementListAdapter adapter,
      ElementListBindingRef listBindingRef,
      /*@Nullable*/ Element optionalBaseElement,
      FrameContext frameContext) {
    Element baseElement =
        optionalBaseElement == null ? Element.getDefaultInstance() : optionalBaseElement;
    BindingValue listBinding = frameContext.getElementListBindingValue(listBindingRef);
    adapter.setVisibility(listBinding.getVisibility());
    if (!listBinding.hasElementList()) {
      if (listBinding.getVisibility() == Visibility.GONE || listBindingRef.getIsOptional()) {
        adapter.setVisibility(Visibility.GONE);
        return;
      } else {
        throw new IllegalArgumentException(
            String.format("ElementList binding %s had no content", listBinding.getBindingId()));
      }
    }
    ElementList list = listBinding.getElementList();
    // If this is a re-bind, we assume this adapter has been released in onUnbindModel
    adapter.createAdapter(list, baseElement, frameContext);
    adapter.bindModel(list, baseElement, frameContext);
  }

  void addChildAdapter(ChildAdapterT adapter) {
    childAdapters.add(adapter);
  }

  @Override
  public void triggerViewActions(View viewport) {
    super.triggerViewActions(viewport);
    for (ElementAdapter<?, ?> childAdapter : childAdapters) {
      childAdapter.triggerViewActions(viewport);
    }
  }
}
