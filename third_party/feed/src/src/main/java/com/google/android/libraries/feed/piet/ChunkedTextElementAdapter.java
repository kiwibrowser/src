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
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.support.annotation.VisibleForTesting;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.ImageSpan;
import android.text.style.StyleSpan;
import android.view.MotionEvent;
import android.view.View;
import android.widget.TextView;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.search.now.ui.piet.ActionsProto.Action;
import com.google.search.now.ui.piet.ActionsProto.Actions;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.StylesProto.Font;
import com.google.search.now.ui.piet.StylesProto.Font.FontWeight;
import com.google.search.now.ui.piet.TextProto.Chunk;
import com.google.search.now.ui.piet.TextProto.ChunkedText;
import com.google.search.now.ui.piet.TextProto.StyledImageChunk;
import java.util.HashSet;
import java.util.Set;

/** An {@link ElementAdapter} which manages {@code ChunkedText} elements. */
class ChunkedTextElementAdapter extends TextElementAdapter {
  private static final String TAG = "ChunkedTextElementAdapter";

  // We only use a LayerDrawable so we can switch out the Drawable; we only want one layer.
  @VisibleForTesting static final int SINGLE_LAYER_ID = 0;

  private final Set<ImageSpanDrawableCallback> loadingImages = new HashSet<>();

  ChunkedTextElementAdapter(Context context, AdapterParameters parameters) {
    super(context, parameters);
  }

  @Override
  void setTextOnView(FrameContext frameContext, TextElement textLine) {
    ChunkedText chunkedText;
    switch (textLine.getContentCase()) {
      case CHUNKED_TEXT:
        chunkedText = textLine.getChunkedText();
        setVisibility(Visibility.VISIBLE);
        break;
      case CHUNKED_TEXT_BINDING:
        BindingValue binding =
            frameContext.getChunkedTextBindingValue(textLine.getChunkedTextBinding());
        setVisibility(binding.getVisibility());
        if (!binding.hasChunkedText()) {
          if (binding.getVisibility() == Visibility.GONE
              || textLine.getChunkedTextBinding().getIsOptional()) {
            setVisibility(Visibility.GONE);
            return;
          } else {
            throw new IllegalArgumentException(
                String.format("Chunked text binding %s had no content", binding.getBindingId()));
          }
        }
        chunkedText = binding.getChunkedText();
        break;
      default:
        throw new IllegalArgumentException(
            String.format(
                "Unhandled type of TextElement; had content %s", textLine.getContentCase()));
    }
    bindChunkedText(chunkedText, frameContext);
  }

  private void bindChunkedText(ChunkedText chunkedText, FrameContext frameContext) {
    TextView textView = getBaseView();
    SpannableStringBuilder spannable = new SpannableStringBuilder();
    boolean hasTouchListener = false;
    for (Chunk chunk : chunkedText.getChunksList()) {
      int chunkStart = spannable.length();
      switch (chunk.getContentCase()) {
        case TEXT_CHUNK:
          addTextChunk(frameContext, spannable, chunk);
          break;
        case IMAGE_CHUNK:
          addImageChunk(frameContext, textView, spannable, chunk);
          break;

        default:
          throw new IllegalArgumentException(
              String.format(
                  "Unhandled type of ChunkedText Chunk; had content %s", chunk.getContentCase()));
      }
      int chunkEnd = spannable.length();
      switch (chunk.getActionsDataCase()) {
        case ACTIONS:
          setChunkActions(
              chunk.getActions(),
              spannable,
              textView,
              frameContext,
              chunkStart,
              chunkEnd,
              hasTouchListener);
          hasTouchListener = true;
          break;
        case ACTIONS_BINDING:
          Actions boundActions = frameContext.getActionsFromBinding(chunk.getActionsBinding());
          if (boundActions == null) {
            break;
          }
          setChunkActions(
              boundActions,
              spannable,
              textView,
              frameContext,
              chunkStart,
              chunkEnd,
              hasTouchListener);
          hasTouchListener = true;
          break;
        default:
          // No actions
      }
    }
    textView.setText(spannable);
  }

  private void setChunkActions(
      Actions actions,
      SpannableStringBuilder spannable,
      TextView textView,
      FrameContext frameContext,
      int chunkStart,
      int chunkEnd,
      boolean hasTouchListener) {
    // TODO: Also support long click actions.
    if (actions.hasOnClickAction()) {
      spannable.setSpan(
          new ActionsClickableSpan(
              actions.getOnClickAction(), frameContext.getActionHandler(), frameContext.getFrame()),
          chunkStart,
          chunkEnd,
          Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      if (!hasTouchListener) {
        textView.setOnTouchListener(new ChunkedTextTouchListener(spannable));
      }
    }
  }

  @VisibleForTesting
  void addTextChunk(FrameContext frameContext, SpannableStringBuilder spannable, Chunk chunk) {
    int start = spannable.length();
    String evaluatedText =
        getParameters()
            .templatedStringEvaluator
            .evaluate(frameContext, chunk.getTextChunk().getParameterizedText());
    spannable.append(evaluatedText);
    StyleProvider chunkStyle = frameContext.makeStyleFor(chunk.getTextChunk().getStyleReferences());
    if (chunkStyle.hasColor()) {
      spannable.setSpan(
          new ForegroundColorSpan(chunkStyle.getColor()),
          start,
          spannable.length(),
          Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }
    if (chunkStyle.getFont().getItalic()) {
      spannable.setSpan(
          new StyleSpan(Typeface.ITALIC),
          start,
          spannable.length(),
          Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }
    if (chunkStyle.getFont().getWeight() == FontWeight.BOLD) {
      spannable.setSpan(
          new StyleSpan(Typeface.BOLD),
          start,
          spannable.length(),
          Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }
    if (chunkStyle.getFont().hasSize()) {
      spannable.setSpan(
          new AbsoluteSizeSpan(
              (int) ViewUtils.dpToPx(chunkStyle.getFont().getSize(), getContext())),
          start,
          spannable.length(),
          Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }
    if (chunkStyle.getMaxLines() > 0) {
      Logger.e(
          TAG,
          "Ignoring max lines parameter set to '%s'; not supported on chunks.",
          chunkStyle.getMaxLines());
    }
  }

  @VisibleForTesting
  void addImageChunk(
      FrameContext frameContext, TextView textView, SpannableStringBuilder spannable, Chunk chunk) {
    StyledImageChunk imageChunk = chunk.getImageChunk();
    StyleProvider chunkStyle = frameContext.makeStyleFor(imageChunk.getStyleReferences());

    // Set a placeholder empty image
    ColorDrawable placeholder = new ColorDrawable(Color.TRANSPARENT);
    LayerDrawable wrapper = new LayerDrawable(new Drawable[] {placeholder});
    wrapper.setId(0, SINGLE_LAYER_ID);
    setBounds(wrapper, chunkStyle, textView);
    ImageSpan imageSpan = new ImageSpan(wrapper);

    spannable.append(" "); // dummy space to overwrite
    spannable.setSpan(
        imageSpan, spannable.length() - 1, spannable.length(), Spannable.SPAN_INCLUSIVE_EXCLUSIVE);

    // Start asynchronously loading the real image.
    ImageSpanDrawableCallback imageSpanLoader =
        new ImageSpanDrawableCallback(wrapper, chunkStyle, textView);
    loadingImages.add(imageSpanLoader);
    getFrameContext().getAssetProvider().getImage(imageChunk.getImage(), imageSpanLoader);
  }

  @Override
  void onUnbindModel() {
    for (ImageSpanDrawableCallback imageLoader : loadingImages) {
      imageLoader.cancelCallback();
    }
    loadingImages.clear();
    super.onUnbindModel();
  }

  @Override
  RecyclerKey createKey(Font font) {
    return KeySupplier.SINGLETON_KEY;
  }

  /**
   * Sets the width and height of the drawable based on the {@code imageStyle}, preserving aspect
   * ratio. If no dimensions are set in the style, defaults to matching the line height of the
   * {@code textView}.
   */
  @VisibleForTesting
  void setBounds(Drawable imageDrawable, StyleProvider imageStyle, TextView textView) {
    int lineHeight = textView.getLineHeight();

    int styleHeight = (int) ViewUtils.dpToPx(imageStyle.getHeight(), getContext());
    int styleWidth = (int) ViewUtils.dpToPx(imageStyle.getWidth(), getContext());

    int height;
    int width;
    if (imageStyle.hasWidth() && imageStyle.hasHeight()) {
      // Scale the image to exactly this size.
      height = styleHeight;
      width = styleWidth;
    } else if (imageStyle.hasWidth()) {
      // Set the width, and preserve aspect ratio.
      width = styleWidth;
      height =
          (int)
              (imageDrawable.getIntrinsicHeight()
                  * ((float) width / imageDrawable.getIntrinsicWidth()));
    } else {
      // Default to making the image the same height as the text, preserving aspect ratio.
      height = imageStyle.hasHeight() ? styleHeight : lineHeight;
      width =
          (int)
              (imageDrawable.getIntrinsicWidth()
                  * ((float) height / imageDrawable.getIntrinsicHeight()));
    }

    imageDrawable.setBounds(0, 0, width, height);
  }

  static class ActionsClickableSpan extends ClickableSpan {
    private final Action action;
    private final ActionHandler handler;
    private final Frame frame;

    ActionsClickableSpan(Action action, ActionHandler handler, Frame frame) {
      this.action = action;
      this.handler = handler;
      this.frame = frame;
    }

    @Override
    public void onClick(View widget) {
      // TODO: Pass VE information with the action.
      handler.handleAction(action, frame, widget, /* veLoggingToken= */ null);
    }

    @Override
    public void updateDrawState(TextPaint textPaint) {
      textPaint.setUnderlineText(false);
    }
  }

  /**
   * Triggers click handlers when text is touched; copied from LinkMovementMethod.onTouchEvent.
   *
   * <p>We can't use LinkMovementMethod because that makes the TextView scrollable (relevant when
   * max_lines is set). We could extend TextView and override onScroll to no-op, but ellipsizing
   * will still be broken in that case.
   */
  static class ChunkedTextTouchListener implements View.OnTouchListener {
    private final SpannableStringBuilder spannable;

    ChunkedTextTouchListener(SpannableStringBuilder spannable) {
      this.spannable = spannable;
    }

    @Override
    public boolean onTouch(View widget, MotionEvent event) {
      int action = event.getAction();

      if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_DOWN) {
        TextView textView = (TextView) widget;
        int x = (int) event.getX();
        int y = (int) event.getY();

        x -= textView.getTotalPaddingLeft();
        y -= textView.getTotalPaddingTop();

        x += textView.getScrollX();
        y += textView.getScrollY();

        Layout layout = textView.getLayout();
        int line = layout.getLineForVertical(y);
        int off = layout.getOffsetForHorizontal(line, x);

        ClickableSpan[] links = spannable.getSpans(off, off, ClickableSpan.class);

        if (links.length != 0) {
          if (action == MotionEvent.ACTION_UP) {
            links[0].onClick(textView);
          }
          return true;
        }
      }
      return false;
    }
  }

  @VisibleForTesting
  class ImageSpanDrawableCallback implements Consumer<Drawable> {
    private final LayerDrawable wrapper;
    private final StyleProvider imageStyle;
    private final TextView textView;
    private boolean cancelled = false;

    ImageSpanDrawableCallback(LayerDrawable wrapper, StyleProvider imageStyle, TextView textView) {
      this.wrapper = wrapper;
      this.imageStyle = imageStyle;
      this.textView = textView;
    }

    @Override
    public void accept(Drawable imageDrawable) {
      if (cancelled) {
        return;
      }
      checkState(
          wrapper.setDrawableByLayerId(SINGLE_LAYER_ID, imageDrawable),
          "Failed to set drawable on chunked text");
      setBounds(imageDrawable, imageStyle, textView);
      textView.invalidate();
      loadingImages.remove(this);
    }

    void cancelCallback() {
      cancelled = true;
    }
  }

  static class KeySupplier extends SingletonKeySupplier<ChunkedTextElementAdapter, TextElement> {
    @Override
    public String getAdapterTag() {
      return TAG;
    }

    @Override
    public ChunkedTextElementAdapter getAdapter(Context context, AdapterParameters parameters) {
      return new ChunkedTextElementAdapter(context, parameters);
    }
  }
}
