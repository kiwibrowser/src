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

import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.piet.DebugLogger.MessageType;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import com.google.search.now.ui.piet.PietProto.Stylesheet;
import com.google.search.now.ui.piet.PietProto.Template;
import com.google.search.now.ui.piet.StylesProto.BoundStyle;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** This class provides support helpers methods for managing styles in Piet. */
public class PietStylesHelper {
  private static final String TAG = "PietStylesHelper";

  private final Map<String, Map<String, Style>> stylesheetScopes = new HashMap<>();
  private final Map<String, Stylesheet> stylesheets = new HashMap<>();
  private final Map<String, Template> templates = new HashMap<>();

  PietStylesHelper(List<PietSharedState> pietSharedStates) {
    setupStylesheetsAndTemplates(pietSharedStates);
  }

  /**
   * Sets up maps of stylesheets and templates from a list of {@link PietSharedState}. If a
   * stylesheet or template id is shared between any number of PietSharedStates, the last
   * stylesheet/template will be used.
   */
  private void setupStylesheetsAndTemplates(
      /*@UnderInitialization*/ PietStylesHelper this, List<PietSharedState> pietSharedStates) {
    for (PietSharedState sharedState : pietSharedStates) {
      if (sharedState.getStylesheetsCount() > 0) {
        for (Stylesheet stylesheet : sharedState.getStylesheetsList()) {
          stylesheets.put(stylesheet.getStylesheetId(), stylesheet);
        }
      }

      for (Template template : sharedState.getTemplatesList()) {
        templates.put(template.getTemplateId(), template);
      }
    }
  }

  /** Returns a Map of style_id to Style. This represents the Stylesheet. */
  Map<String, Style> getStylesheet(String stylesheetId) {
    if (stylesheetScopes.containsKey(stylesheetId)) {
      return stylesheetScopes.get(stylesheetId);
    }
    Stylesheet stylesheet = stylesheets.get(stylesheetId);
    if (stylesheet != null) {
      Map<String, Style> stylesheetMap = createMapFromStylesheet(stylesheet);
      stylesheetScopes.put(stylesheet.getStylesheetId(), stylesheetMap);
      return stylesheetMap;
    }
    Logger.w(TAG, "Stylesheet [%s] was not found in the Stylesheet", stylesheetId);
    return createMapFromStylesheet(Stylesheet.getDefaultInstance());
  }

  static Map<String, Style> createMapFromStylesheet(Stylesheet stylesheet) {
    Map<String, Style> stylesheetMap = new HashMap<>();
    for (Style style : stylesheet.getStylesList()) {
      stylesheetMap.put(style.getStyleId(), style);
    }
    return stylesheetMap;
  }

  /** Returns a {@link Template} for the template */
  /*@Nullable*/
  public Template getTemplate(String templateId) {
    return templates.get(templateId);
  }

  /**
   * Given a StyleIdsStack, a base style, and a styleMap that contains the styles definition,
   * returns a Style that is the proto-merge of all the styles in the stack, starting with the base.
   */
  static Style mergeStyleIdsStack(
      Style baseStyle,
      StyleIdsStack stack,
      Map<String, Style> styleMap,
      /*@Nullable*/ FrameContext frameContext) {
    Style.Builder mergedStyle = baseStyle.toBuilder();
    for (String style : stack.getStyleIdsList()) {
      if (styleMap.containsKey(style)) {
        mergedStyle.mergeFrom(styleMap.get(style)).build();
      } else {
        String error =
            String.format("Unable to bind style [%s], style not found in Stylesheet", style);
        if (frameContext != null) {
          frameContext.reportError(MessageType.ERROR, error);
        }
        Logger.w(TAG, error);
      }
    }
    if (stack.hasStyleBinding()) {
      FrameContext localFrameContext =
          checkNotNull(frameContext, "Binding styles not supported when frameContext is null");
      BoundStyle boundStyle = localFrameContext.getStyleFromBinding(stack.getStyleBinding());
      if (boundStyle.hasBackground()) {
        mergedStyle.setBackground(
            mergedStyle.getBackground().toBuilder().mergeFrom(boundStyle.getBackground()));
      }
      if (boundStyle.hasColor()) {
        mergedStyle.setColor(boundStyle.getColor());
      }
    }
    return mergedStyle.build();
  }
}
