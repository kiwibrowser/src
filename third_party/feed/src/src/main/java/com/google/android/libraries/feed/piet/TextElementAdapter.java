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
import android.graphics.Typeface;
import android.os.Build;
import android.support.annotation.VisibleForTesting;
import android.support.v4.widget.TextViewCompat;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;

/**
 * Base {@link ElementAdapter} to extend to manage {@code ChunkedText} and {@code ParameterizedText}
 * elements.
 */
abstract class TextElementAdapter extends ElementAdapter<TextView, TextElement> {
  private static final String TAG = "TextElementAdapter";

  TextElementAdapter(Context context, AdapterParameters parameters) {
    super(context, parameters, new TextView(context));
  }

  @Override
  protected TextElement getModelFromElement(Element baseElement) {
    if (!baseElement.hasTextElement()) {
      throw new IllegalArgumentException(
          String.format("Missing TextElement; has %s", baseElement.getElementsCase()));
    }
    return baseElement.getTextElement();
  }

  @Override
  void onCreateAdapter(TextElement textLine, Element baseElement, FrameContext frameContext) {
    if (getKey() == null) {
      Font font = getElementStyle().getFont();
      setFont(font);
      setKey(createKey(font));
    }

    // Setup the layout of the text lines.
    updateTextStyle();
  }

  private void updateTextStyle() {
    TextView textView = getBaseView();
    StyleProvider textStyle = getElementStyle();
    textView.setTextColor(textStyle.getColor());
    if (textStyle.getMaxLines() > 0) {
      textView.setMaxLines(textStyle.getMaxLines());
      textView.setEllipsize(TextUtils.TruncateAt.END);
    } else {
      // MAX_VALUE is the value used in the Android implementation for the default
      textView.setMaxLines(Integer.MAX_VALUE);
    }
    switch (getHorizontalGravity()) {
      case GRAVITY_START:
        textView.setGravity(Gravity.START);
        break;
      case GRAVITY_CENTER:
        textView.setGravity(Gravity.CENTER_HORIZONTAL);
        break;
      case GRAVITY_END:
        textView.setGravity(Gravity.END);
        break;
      default:
    }
  }

  @Override
  void onBindModel(TextElement textLine, Element baseElement, FrameContext frameContext) {

    // Set the initial state for the TextView
    // No bindings found, so use the inlined value (or empty if not set)
    setTextOnView(frameContext, textLine);

    if (textLine.getStyleReferences().hasStyleBinding()) {
      updateTextStyle();
    }
  }

  @Override
  StyleIdsStack getElementStyleIdsStack() {
    return getModel().getStyleReferences();
  }

  abstract void setTextOnView(FrameContext frameContext, TextElement textElement);

  @Override
  void onUnbindModel() {
    TextView textView = getBaseView();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      textView.setTextAlignment(View.TEXT_ALIGNMENT_GRAVITY);
    }
    textView.setVisibility(View.VISIBLE);
    textView.setText("");
  }

  abstract RecyclerKey createKey(Font font);

  @VisibleForTesting
  void setFont(Font font) {
    // TODO: Line Height is currently not supported.
    // TODO: Implement typefaces
    TextView textView = getBaseView();
    textView.setTextSize(font.getSize());
    boolean bold = false;
    switch (font.getWeight()) {
      case REGULAR:
        TextViewCompat.setTextAppearance(textView, R.style.gm_font_weight_regular);
        break;
      case THIN:
        TextViewCompat.setTextAppearance(textView, R.style.gm_font_weight_thin);
        break;
      case LIGHT:
        TextViewCompat.setTextAppearance(textView, R.style.gm_font_weight_light);
        break;
      case MEDIUM:
        TextViewCompat.setTextAppearance(textView, R.style.gm_font_weight_medium);
        break;
      case BOLD:
        bold = true;
        TextViewCompat.setTextAppearance(textView, R.style.gm_font_weight_regular);
        break;
      case BLACK:
        bold = true;
        TextViewCompat.setTextAppearance(textView, R.style.gm_font_weight_black);
        break;
      default:
        TextViewCompat.setTextAppearance(textView, R.style.gm_font_weight_regular);
        break;
    }
    if (bold || font.getItalic()) {
      if (bold && font.getItalic()) {
        textView.setTypeface(textView.getTypeface(), Typeface.BOLD_ITALIC);
      } else if (bold) {
        textView.setTypeface(textView.getTypeface(), Typeface.BOLD);
      } else if (font.getItalic()) {
        textView.setTypeface(textView.getTypeface(), Typeface.ITALIC);
      }
    } else {
      textView.setTypeface(Typeface.create(textView.getTypeface(), Typeface.NORMAL));
    }
  }
}
