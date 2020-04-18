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
import android.graphics.Canvas;
import android.graphics.drawable.ColorDrawable;
import android.support.annotation.VisibleForTesting;
import android.view.View;
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.android.libraries.feed.piet.SpacerElementAdapter.DividerView;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.SpacerElement;
import com.google.search.now.ui.piet.StylesProto.EdgeWidths;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;

/** An {@link ElementAdapter} for {@code SpacerElement} elements. */
class SpacerElementAdapter extends ElementAdapter<DividerView, SpacerElement> {
  private static final String TAG = "SpacerElementAdapter";

  @VisibleForTesting
  SpacerElementAdapter(Context context, AdapterParameters parameters) {
    super(context, parameters, new DividerView(context), KeySupplier.SINGLETON_KEY);
  }

  @Override
  SpacerElement getModelFromElement(Element baseElement) {
    if (!baseElement.hasSpacerElement()) {
      throw new IllegalArgumentException(
          String.format("Missing SpacerElement; has %s", baseElement.getElementsCase()));
    }
    return baseElement.getSpacerElement();
  }

  @Override
  void onCreateAdapter(SpacerElement model, Element baseElement, FrameContext frameContext) {
    StyleProvider styleProvider = getElementStyle();
    getBaseView()
        .initialize(styleProvider.getColor(), model.getHeight(), styleProvider.getPadding());
    setSpacerStyles(frameContext);
  }

  @Override
  void onBindModel(SpacerElement model, Element baseElement, FrameContext frameContext) {
    if (model.getStyleReferences().hasStyleBinding()) {
      setSpacerStyles(frameContext);
    }
  }

  private void setSpacerStyles(FrameContext frameContext) {
    StyleProvider spacerStyle = frameContext.makeStyleFor(getModel().getStyleReferences());
    EdgeWidths padding = spacerStyle.getPadding();

    int height = padding.getTop() + padding.getBottom() + getModel().getHeight();
    heightPx = (int) ViewUtils.dpToPx(height, getContext());
  }

  @Override
  StyleIdsStack getElementStyleIdsStack() {
    return getModel().getStyleReferences();
  }

  /**
   * This is a subclass of View which can draw a spacer within the View. Consider it an empty view
   * which can have a height & Styles. This can be used to create empty areas, dividers, horizontal
   * rules & blocks of color.
   */
  static class DividerView extends View {
    private ColorDrawable spacerDrawable;
    private int heightInPx;
    private int topPaddingInPx;
    private final Context context;

    public DividerView(Context context) {
      super(context);
      this.context = context;
    }

    public void initialize(int color, int height, EdgeWidths padding) {
      spacerDrawable = new ColorDrawable(color);
      heightInPx = (int) ViewUtils.dpToPx(height, context);
      this.topPaddingInPx = (int) ViewUtils.dpToPx(padding.getTop(), context);
    }

    @Override
    protected void onDraw(Canvas canvas) {
      super.onDraw(canvas);
      if (heightInPx > 0) {
        int paddingLeft = getPaddingLeft();
        int paddingRight = getPaddingRight();
        int measuredWidth = getMeasuredWidth();
        spacerDrawable.setBounds(
            paddingLeft, topPaddingInPx, measuredWidth - paddingRight, topPaddingInPx + heightInPx);
        spacerDrawable.draw(canvas);
      }
    }
  }

  static class KeySupplier extends SingletonKeySupplier<SpacerElementAdapter, SpacerElement> {
    @Override
    public String getAdapterTag() {
      return TAG;
    }

    @Override
    public SpacerElementAdapter getAdapter(Context context, AdapterParameters parameters) {
      return new SpacerElementAdapter(context, parameters);
    }
  }
}
