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
import android.view.View;
import android.widget.FrameLayout;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.CustomElement;
import com.google.search.now.ui.piet.ElementsProto.CustomElementData;
import com.google.search.now.ui.piet.ElementsProto.Element;

/** Adapter that manages a custom view created by the host. */
class CustomElementAdapter extends ElementAdapter<FrameLayout, CustomElement> {
  private static final String TAG = "CustomElementAdapter";

  CustomElementAdapter(Context context, AdapterParameters parameters) {
    super(context, parameters, new FrameLayout(context), KeySupplier.SINGLETON_KEY);
  }

  @Override
  protected CustomElement getModelFromElement(Element baseElement) {
    if (!baseElement.hasCustomElement()) {
      throw new IllegalArgumentException(
          String.format("Missing CustomElement; has %s", baseElement.getElementsCase()));
    }
    return baseElement.getCustomElement();
  }

  @Override
  void onBindModel(CustomElement customElement, Element baseElement, FrameContext frameContext) {
    CustomElementData customElementData;
    switch (customElement.getContentCase()) {
      case CUSTOM_ELEMENT_DATA:
        customElementData = customElement.getCustomElementData();
        setVisibility(Visibility.VISIBLE);
        break;
      case CUSTOM_BINDING:
        BindingValue binding =
            frameContext.getCustomElementBindingValue(customElement.getCustomBinding());
        setVisibility(binding.getVisibility());
        if (!binding.hasCustomElementData()) {
          if (binding.getVisibility() == Visibility.GONE
              || customElement.getCustomBinding().getIsOptional()) {
            setVisibility(Visibility.GONE);
            return;
          } else {
            throw new IllegalArgumentException(
                String.format("Custom element binding %s had no content", binding.getBindingId()));
          }
        }
        customElementData = binding.getCustomElementData();
        break;
      default:
        Logger.e(TAG, "Missing payload in CustomElement");
        return;
    }

    View v = frameContext.getCustomElementProvider().createCustomElement(customElementData);
    getBaseView().addView(v);
  }

  @Override
  void onUnbindModel() {
    FrameLayout baseView = getBaseView();
    if (baseView != null && baseView.getChildCount() > 0) {
      for (int i = 0; i < baseView.getChildCount(); i++) {
        getFrameContext().getCustomElementProvider().releaseCustomView(baseView.getChildAt(i));
      }
      baseView.removeAllViews();
    }
  }

  static class KeySupplier extends SingletonKeySupplier<CustomElementAdapter, CustomElement> {
    @Override
    public String getAdapterTag() {
      return TAG;
    }

    @Override
    public CustomElementAdapter getAdapter(Context context, AdapterParameters parameters) {
      return new CustomElementAdapter(context, parameters);
    }
  }
}
