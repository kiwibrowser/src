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
import android.text.Html;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import java.util.ArrayList;
import java.util.IllegalFormatException;
import java.util.List;
import java.util.concurrent.TimeUnit;

/** Helper class to format templated strings when rendering cards. */
public class ParameterizedTextEvaluator {
  private static final String TAG = "ParameterizedTextEvalua";

  /**
   * Evaluates the given parameterized string, and returns an html formatted char sequence for
   * display.
   *
   * @return the evaluated template, or {@code null} if there is no display string
   */
  /*@Nullable*/
  public CharSequence evaluateHtml(
      FrameContext frameContext, Context context, ParameterizedText templatedString) {
    if (!templatedString.hasText()) {
      Logger.w(TAG, "Got templated string with no display string");
      return null;
    }

    String evaluated = evaluate(frameContext, templatedString);
    if (evaluated == null) {
      return null;
    }

    return Html.fromHtml(evaluated);
  }

  /**
   * Evaluates the given ParameterizedText, and returns the raw evaluated value, without any Html
   * wrapping.
   */
  public String evaluate(FrameContext frameContext, ParameterizedText templatedString) {

    if (!templatedString.hasText()) {
      Logger.w(TAG, "Got templated string with no display string");
      return templatedString.getText();
    }

    if (templatedString.getParametersCount() == 0) {
      return templatedString.getText();
    }

    List<String> params = new ArrayList<>();
    for (ParameterizedText.Parameter param : templatedString.getParametersList()) {
      String value = evaluateParam(frameContext, param);
      // Add a placeholder for any invalid parameters so that it will be obvious which one
      // failed.
      if (value == null) {
        value = "(invalid param)";
      }
      params.add(value);
    }

    String displayString = templatedString.getText();
    try {
      return String.format(displayString, (Object[]) params.toArray(new String[params.size()]));
    } catch (IllegalFormatException e) {
      // Don't crash if we get invalid data - just log the display string and the error.
      Logger.e(TAG, e, "Error formatting display string \"%s\"", displayString);
      return displayString;
    }
  }

  /*@Nullable*/
  private String evaluateParam(FrameContext frameContext, ParameterizedText.Parameter param) {
    if (param.hasTimestampSeconds()) {
      long elapsedTimeMillis =
          getCurrentTimeMillis() - TimeUnit.SECONDS.toMillis(param.getTimestampSeconds());
      return frameContext.getAssetProvider().getRelativeElapsedString(elapsedTimeMillis);
    }
    return null;
  }

  @VisibleForTesting
  long getCurrentTimeMillis() {
    return System.currentTimeMillis();
  }
}
