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
import android.widget.TextView;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.piet.AdapterFactory.AdapterKeySupplier;
import com.google.android.libraries.feed.piet.DebugLogger.MessageType;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.Font.FontWeight;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;

/** An {@link ElementAdapter} which manages {@code ParameterizedText} elements. */
class ParameterizedTextElementAdapter extends TextElementAdapter {
  private static final String TAG = "TextElementAdapter";

  ParameterizedTextElementAdapter(Context context, AdapterParameters parameters) {
    super(context, parameters);
  }

  @Override
  void setTextOnView(FrameContext frameContext, TextElement textLine) {
    switch (textLine.getContentCase()) {
      case PARAMETERIZED_TEXT:
        // No bindings found, so use the inlined value (or empty if not set)
        setTextOnView(frameContext, getBaseView(), textLine.getParameterizedText());
        setVisibility(Visibility.VISIBLE);
        break;
      case PARAMETERIZED_TEXT_BINDING:
        BindingValue bindingValue =
            frameContext.getParameterizedTextBindingValue(textLine.getParameterizedTextBinding());
        if (bindingValue == null) {
          // If no binding is specified, raise an error and hide the corresponding UI element.
          Logger.e(
              TAG,
              frameContext.reportError(
                  MessageType.ERROR,
                  String.format(
                      "TextElement Binding not found for %s",
                      textLine.getParameterizedTextBinding())));
          getBaseView().setVisibility(View.GONE);
          return;
        }

        setVisibility(bindingValue.getVisibility());
        if (!bindingValue.hasParameterizedText()
            && !textLine.getParameterizedTextBinding().getIsOptional()
            && bindingValue.getVisibility() != Visibility.GONE) {
          throw new IllegalArgumentException(
              String.format(
                  "Parameterized text binding %s had no content", bindingValue.getBindingId()));
        }

        // Always set text even if visibility is set to GONE or INVISIBLE,
        // in case a later optimistic update operation changes the visibility but wants to retain
        // the previously-set text.
        setTextOnView(frameContext, getBaseView(), bindingValue.getParameterizedText());
        break;
      default:
        throw new IllegalArgumentException(
            String.format(
                "TextElement missing ParameterizedText content; has %s",
                textLine.getContentCase()));
    }
  }

  void setTextOnView(
      FrameContext frameContext, TextView textView, ParameterizedText parameterizedText) {
    if (!parameterizedText.hasText()) {
      Logger.e(
          TAG,
          frameContext.reportError(
              MessageType.ERROR,
              "ParameterizedText missing for TextElement, setting to empty String"));
      textView.setText("");
      return;
    }

    textView.setText(getTemplatedStringEvaluator().evaluate(frameContext, parameterizedText));
  }

  @Override
  RecyclerKey createKey(Font font) {
    return new Key(font);
  }

  static class KeySupplier
      implements AdapterKeySupplier<ParameterizedTextElementAdapter, TextElement> {
    @Override
    public String getAdapterTag() {
      return TAG;
    }

    @Override
    public ParameterizedTextElementAdapter getAdapter(
        Context context, AdapterParameters parameters) {
      return new ParameterizedTextElementAdapter(context, parameters);
    }

    @Override
    public Key getKey(FrameContext frameContext, TextElement model) {
      StyleProvider styleProvider = frameContext.makeStyleFor(model.getStyleReferences());
      return new Key(styleProvider.getFont());
    }
  }

  /** We will Key TextViews off of the Ellipsizing, Font Size and FontWeight, and Italics. */
  static class Key extends RecyclerKey {
    private int size;
    private FontWeight fontWeight;
    private boolean italic;

    Key(Font font) {
      size = font.getSize();
      fontWeight = font.getWeight();
      italic = font.getItalic();
    }

    @Override
    public int hashCode() {
      // Can't use Objects.hash() as it is only available in KK+ and can't use Guava's impl either.
      int result = size;
      result = 31 * result + fontWeight.getNumber();
      result = 31 * result + (italic ? 1 : 0);
      return result;
    }

    @Override
    public boolean equals(/*@Nullable*/ Object obj) {
      if (obj == this) {
        return true;
      }

      if (obj == null) {
        return false;
      }

      if (!(obj instanceof Key)) {
        return false;
      }

      Key key = (Key) obj;
      return key.size == size && key.fontWeight == fontWeight && key.italic == italic;
    }
  }
}
