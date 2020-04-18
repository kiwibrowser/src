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

import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE;
import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE_PROVIDER;

import android.content.Context;
import android.graphics.drawable.Drawable;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.piet.DebugLogger.MessageType;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.android.libraries.feed.piet.ui.RoundedCornerColorDrawable;
import com.google.search.now.ui.piet.ActionsProto.Actions;
import com.google.search.now.ui.piet.BindingRefsProto.ActionsBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ChunkedTextBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.CustomBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ElementListBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.GridCellWidthBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ImageBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ParameterizedTextBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.StyleBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.TemplateBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingContext;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.GridCellWidth;
import com.google.search.now.ui.piet.ElementsProto.TemplateInvocation;
import com.google.search.now.ui.piet.GradientsProto.Fill;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import com.google.search.now.ui.piet.PietProto.Template;
import com.google.search.now.ui.piet.RoundedCornersProto.RoundedCorners;
import com.google.search.now.ui.piet.StylesProto.BoundStyle;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This is a {@link FrameContext} that is bound. Binding means we have a {@code Frame}, an optional
 * map of {@code BindingValues} and a {@code StyleProvider}.
 */
class FrameContext {
  private static final String TAG = "FrameContext";

  private final PietStylesHelper stylesHelper;
  private final List<PietSharedState> pietSharedStates;
  private final DebugBehavior debugBehavior;
  private final DebugLogger debugLogger;
  private final AssetProvider assetProvider;
  private final CustomElementProvider customElementProvider;
  private final HostBindingProvider hostBindingProvider;
  private final ActionHandler actionHandler;
  private final Map<String, Template> frameTemplates = new HashMap<>();

  // This is the Frame which contains all the slices being processed.
  private final Frame currentFrame;

  // The Current Stylesheet as a map from style_id to Style.
  private final Map<String, Style> stylesheet;

  // The in-scope bindings as a map from state_id to binding value
  private final Map<String, BindingValue> bindingValues;

  // This is the current style provider
  private final StyleProvider currentStyleProvider;

  // The base / default style; inherited from the template or frame.
  private final Style baseStyle;

  FrameContext(
      Frame frame,
      Map<String, Style> stylesheet,
      Style baseStyle,
      StyleProvider styleProvider,
      List<PietSharedState> pietSharedStates,
      PietStylesHelper pietStylesHelper,
      DebugBehavior debugBehavior,
      DebugLogger debugLogger,
      AssetProvider assetProvider,
      CustomElementProvider customElementProvider,
      HostBindingProvider hostBindingProvider,
      ActionHandler actionHandler) {
    this(
        frame,
        stylesheet,
        new ThrowingEmptyMap(),
        baseStyle,
        styleProvider,
        pietSharedStates,
        pietStylesHelper,
        debugBehavior,
        debugLogger,
        assetProvider,
        customElementProvider,
        hostBindingProvider,
        actionHandler);
  }

  FrameContext(
      Frame frame,
      Map<String, Style> stylesheet,
      Map<String, BindingValue> bindingValues,
      Style baseStyle,
      StyleProvider styleProvider,
      List<PietSharedState> pietSharedStates,
      PietStylesHelper pietStylesHelper,
      DebugBehavior debugBehavior,
      DebugLogger debugLogger,
      AssetProvider assetProvider,
      CustomElementProvider customElementProvider,
      HostBindingProvider hostBindingProvider,
      ActionHandler actionHandler) {
    currentFrame = frame;
    this.stylesheet = stylesheet;
    this.bindingValues = bindingValues;
    this.baseStyle = baseStyle;
    currentStyleProvider = styleProvider;
    this.stylesHelper = pietStylesHelper;
    this.pietSharedStates = pietSharedStates;
    this.debugBehavior = debugBehavior;
    this.debugLogger = debugLogger;
    this.assetProvider = assetProvider;
    this.customElementProvider = customElementProvider;
    this.hostBindingProvider = hostBindingProvider;
    this.actionHandler = actionHandler;

    if (frame.getTemplatesCount() > 0) {
      for (Template template : frame.getTemplatesList()) {
        frameTemplates.put(template.getTemplateId(), template);
      }
    }
  }

  /**
   * Creates a {@link FrameContext} and constructs the stylesheets from the frame and the list of
   * {@link PietSharedState}. Any errors found with the styling will be reported.
   */
  public static FrameContext createFrameContext(
      Frame frame,
      Style baseStyle,
      List<PietSharedState> pietSharedStates,
      DebugBehavior debugBehavior,
      DebugLogger debugLogger,
      AssetProvider assetProvider,
      CustomElementProvider customElementProvider,
      HostBindingProvider hostBindingProvider,
      ActionHandler actionHandler) {
    PietStylesHelper pietStylesHelper = new PietStylesHelper(pietSharedStates);
    Map<String, Style> stylesheetMap = null;
    if (frame.hasStylesheetId()) {
      stylesheetMap = pietStylesHelper.getStylesheet(frame.getStylesheetId());
    } else if (frame.hasStylesheet()) {
      stylesheetMap = PietStylesHelper.createMapFromStylesheet(frame.getStylesheet());
    }

    Style frameStyle = null;
    if (frame.hasStyleReferences()) {
      if (stylesheetMap != null) {
        frameStyle =
            PietStylesHelper.mergeStyleIdsStack(
                DEFAULT_STYLE, frame.getStyleReferences(), stylesheetMap, null);
      }
    }

    StyleProvider styleProvider =
        (frameStyle != null) ? new StyleProvider(frameStyle) : StyleProvider.DEFAULT_STYLE_PROVIDER;
    if (stylesheetMap == null) {
      stylesheetMap = new HashMap<>();
    }
    FrameContext frameContext =
        new FrameContext(
            frame,
            stylesheetMap,
            baseStyle,
            styleProvider,
            pietSharedStates,
            pietStylesHelper,
            debugBehavior,
            debugLogger,
            assetProvider,
            customElementProvider,
            hostBindingProvider,
            actionHandler);
    frameContext.computeErrors(frameStyle);

    return frameContext;
  }

  public DebugBehavior getDebugBehavior() {
    return debugBehavior;
  }

  public DebugLogger getDebugLogger() {
    return debugLogger;
  }

  public AssetProvider getAssetProvider() {
    return assetProvider;
  }

  public CustomElementProvider getCustomElementProvider() {
    return customElementProvider;
  }

  public Frame getFrame() {
    return currentFrame;
  }

  public ActionHandler getActionHandler() {
    return actionHandler;
  }

  /*@Nullable*/
  public Template getTemplate(String templateId) {
    Template template = frameTemplates.get(templateId);
    return (template != null) ? template : stylesHelper.getTemplate(templateId);
  }

  public List<PietSharedState> getPietSharedStates() {
    return pietSharedStates;
  }

  /**
   * Creates a new FrameContext which is scoped properly for the Template. The Frame's stylesheet is
   * discarded and replaced by the Template's stylesheet. In addition, a Template can define a root
   * level style which applies to all child elements.
   */
  // TODO: Why do we allow bindingContext to be null if it's an error?
  FrameContext bindTemplate(Template template, /*@Nullable*/ BindingContext bindingContext) {
    if (bindingContext == null) {
      Logger.w(TAG, reportError(MessageType.WARNING, "Template BindingContext was not provided"));
      return this;
    }
    Map<String, BindingValue> binding = createBindingValueMap(bindingContext);
    Map<String, Style> localStylesheet;

    switch (template.getTemplateStylesheetCase()) {
      case STYLESHEET:
        localStylesheet = PietStylesHelper.createMapFromStylesheet(template.getStylesheet());
        break;
      case STYLESHEET_ID:
        localStylesheet = stylesHelper.getStylesheet(template.getStylesheetId());
        break;
      default:
        localStylesheet = new HashMap<>();
    }

    Style templateBaseStyle;
    StyleProvider templateStyleProvider;
    if (template.hasChildDefaultStyleIds()) {
      templateBaseStyle =
          PietStylesHelper.mergeStyleIdsStack(
              DEFAULT_STYLE, template.getChildDefaultStyleIds(), localStylesheet, this);
      templateStyleProvider = new StyleProvider(templateBaseStyle);
    } else {
      templateBaseStyle = DEFAULT_STYLE;
      templateStyleProvider = DEFAULT_STYLE_PROVIDER;
    }
    return new FrameContext(
        currentFrame,
        localStylesheet,
        binding,
        templateBaseStyle,
        templateStyleProvider,
        pietSharedStates,
        stylesHelper,
        debugBehavior,
        debugLogger,
        assetProvider,
        customElementProvider,
        hostBindingProvider,
        actionHandler);
  }

  /**
   * Creates a new FrameContext from the existing FrameContext using {@code styleId} to lookup the
   * new Style.
   */
  FrameContext bindNewStyle(StyleIdsStack styles) {
    StyleProvider styleProvider = makeStyleFor(styles);
    return new FrameContext(
        currentFrame,
        stylesheet,
        bindingValues,
        baseStyle,
        styleProvider,
        pietSharedStates,
        stylesHelper,
        debugBehavior,
        debugLogger,
        assetProvider,
        customElementProvider,
        hostBindingProvider,
        actionHandler);
  }

  /** Return the current {@link StyleProvider}. */
  public StyleProvider getCurrentStyle() {
    return currentStyleProvider;
  }

  /** Return a {@link StyleProvider} for the style. */
  public StyleProvider makeStyleFor(StyleIdsStack styles) {
    return new StyleProvider(
        PietStylesHelper.mergeStyleIdsStack(baseStyle, styles, stylesheet, this));
  }

  /**
   * Return a {@link Drawable} with the fill and rounded corners defined on the style; returns
   * {@code null} if the background has no color defined.
   */
  /*@Nullable*/
  Drawable createBackground(StyleProvider style, Context context) {
    Fill background = style.getBackground();
    if (!background.hasColor()) {
      return null;
    }
    RoundedCornerColorDrawable bgDrawable = new RoundedCornerColorDrawable(background.getColor());
    if (style.hasRoundedCorners()) {
      bgDrawable.setRoundedCorners(style.getRoundedCorners().getBitmask());
      bgDrawable.setRadius(getRoundedCornerRadius(style, context));
    }
    return bgDrawable;
  }

  /**
   * Return a {@link GridCellWidth} for the binding if there is one defined; otherwise returns
   * {@code null}.
   */
  /*@Nullable*/
  GridCellWidth getGridCellWidthFromBinding(GridCellWidthBindingRef binding) {
    BindingValue bindingValue = bindingValues.get(binding.getBindingId());
    // Purposefully check for host binding and overwrite here as we want to perform the hasCellWidth
    // checks on host binding.   This allows the host to act more like the server.
    if (bindingValue != null && bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getGridCellWidthBindingForValue(bindingValue);
    }
    return bindingValue == null || !bindingValue.hasCellWidth()
        ? null
        : bindingValue.getCellWidth();
  }

  /**
   * Return an {@link Actions} for the binding if there is one defined; otherwise returns the
   * default instance.
   */
  Actions getActionsFromBinding(ActionsBindingRef binding) {
    BindingValue bindingValue = bindingValues.get(binding.getBindingId());
    if (bindingValue != null && bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getActionsBindingForValue(bindingValue);
    }
    if (bindingValue == null || !bindingValue.hasActions()) {
      Logger.w(TAG, "No actions found for binding %s", binding.getBindingId());
      return Actions.getDefaultInstance();
    }
    return bindingValue.getActions();
  }

  /**
   * Return a {@link BoundStyle} for the binding if there is one defined; otherwise returns the
   * default instance.
   */
  BoundStyle getStyleFromBinding(StyleBindingRef binding) {
    BindingValue bindingValue = bindingValues.get(binding.getBindingId());
    if (bindingValue != null && bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getStyleBindingForValue(bindingValue);
    }
    if (bindingValue == null || !bindingValue.hasBoundStyle()) {
      Logger.w(TAG, "No style found for binding %s", binding.getBindingId());
      return BoundStyle.getDefaultInstance();
    }
    return bindingValue.getBoundStyle();
  }

  /**
   * Return a {@link TemplateInvocation} for the binding if there is one defined; otherwise returns
   * {@code null}.
   */
  /*@Nullable*/
  TemplateInvocation getTemplateInvocationFromBinding(TemplateBindingRef binding) {
    BindingValue bindingValue = bindingValues.get(binding.getBindingId());
    if (bindingValue != null && bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getTemplateBindingForValue(bindingValue);
    }
    if (bindingValue != null && bindingValue.hasVisibility()) {
      Logger.e(TAG, "Visibility not supported for templates; got %s", bindingValue.getVisibility());
    }
    return bindingValue == null || !bindingValue.hasTemplateInvocation()
        ? null
        : bindingValue.getTemplateInvocation();
  }

  /**
   * Returns the {@link BindingValue} for the BindingRef; throws IllegalStateException if binding id
   * does not point to a custom element resource.
   */
  BindingValue getCustomElementBindingValue(CustomBindingRef binding) {
    BindingValue bindingValue = getBindingValue(binding.getBindingId());
    if (bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getCustomElementDataBindingForValue(bindingValue);
    }
    if (!bindingValue.hasCustomElementData() && !binding.getIsOptional()) {
      throw new IllegalStateException(
          reportError(
              MessageType.ERROR,
              String.format("Custom element binding not found for %s", binding.getBindingId())));
    } else {
      return bindingValue;
    }
  }

  /**
   * Returns the {@link BindingValue} for the BindingRef; throws IllegalStateException if binding id
   * does not point to a chunked text resource.
   */
  BindingValue getChunkedTextBindingValue(ChunkedTextBindingRef binding) {
    BindingValue bindingValue = getBindingValue(binding.getBindingId());
    if (bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getChunkedTextBindingForValue(bindingValue);
    }
    if (!bindingValue.hasChunkedText() && !binding.getIsOptional()) {
      throw new IllegalStateException(
          reportError(
              MessageType.ERROR,
              String.format("Chunked text binding not found for %s", binding.getBindingId())));
    } else {
      return bindingValue;
    }
  }

  /**
   * Returns the {@link BindingValue} for the BindingRef; throws IllegalStateException if binding id
   * does not point to a parameterized text resource.
   */
  BindingValue getParameterizedTextBindingValue(ParameterizedTextBindingRef binding) {
    BindingValue bindingValue = getBindingValue(binding.getBindingId());
    if (bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getParameterizedTextBindingForValue(bindingValue);
    }
    if (!bindingValue.hasParameterizedText() && !binding.getIsOptional()) {
      throw new IllegalStateException(
          reportError(
              MessageType.ERROR,
              String.format(
                  "Parameterized text binding not found for %s", binding.getBindingId())));
    } else {
      return bindingValue;
    }
  }

  /**
   * Returns the {@link BindingValue} for the BindingRef; throws IllegalStateException if binding id
   * does not point to a image resource.
   */
  BindingValue getImageBindingValue(ImageBindingRef binding) {
    BindingValue bindingValue = getBindingValue(binding.getBindingId());
    if (bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getImageBindingForValue(bindingValue);
    }
    if (!bindingValue.hasImage() && !binding.getIsOptional()) {
      throw new IllegalStateException(
          reportError(
              MessageType.ERROR,
              String.format("Image binding not found for %s", binding.getBindingId())));
    } else {
      return bindingValue;
    }
  }

  /**
   * Returns the {@link BindingValue} for the BindingRef; throws IllegalStateException if binding id
   * does not point to a ElementList.
   */
  BindingValue getElementListBindingValue(ElementListBindingRef binding) {
    BindingValue bindingValue = getBindingValue(binding.getBindingId());
    if (bindingValue.hasHostBindingData()) {
      bindingValue = hostBindingProvider.getElementListBindingForValue(bindingValue);
    }
    if (!bindingValue.hasElementList() && !binding.getIsOptional()) {
      throw new IllegalStateException(
          reportError(
              MessageType.ERROR,
              String.format("ElementList binding not found for %s", binding.getBindingId())));
    } else {
      return bindingValue;
    }
  }

  private BindingValue getBindingValue(String bindingId) {
    BindingValue returnValue = bindingValues.get(bindingId);
    if (returnValue == null) {
      return BindingValue.getDefaultInstance();
    }
    return returnValue;
  }

  /**
   * Report user errors in frames. This will return the fully formed error so it can be logged at
   * the site of the error.
   */
  public String reportError(@MessageType int errorType, String error) {
    String e = String.format("[%s] %s", currentFrame.getTag(), error);
    debugLogger.recordMessage(errorType, e);
    return e;
  }

  private void computeErrors(/*@Nullable*/ Style frameStyle) {
    if (currentFrame.hasStylesheetId() && stylesheet.isEmpty()) {
      String error =
          String.format(
              "Stylesheet [%s] not found, no stylesheet used", currentFrame.getStylesheetId());
      Logger.w(TAG, reportError(MessageType.ERROR, error));
    } else if (!currentFrame.hasStylesheetId() && !currentFrame.hasStylesheet()) {
      String error = "Frame defined without any Stylesheet";
      Logger.w(TAG, reportError(MessageType.ERROR, error));
    }

    if (currentFrame.hasStyleReferences() && frameStyle == null) {
      String error =
          String.format(
              "Couldn't find frame styles [%s] in the Stylesheet",
              currentFrame.getStyleReferences());
      Logger.e(TAG, reportError(MessageType.ERROR, error));
    }
  }

  public int getRoundedCornerRadius(StyleProvider style, Context context) {
    RoundedCorners roundedCorners = style.getRoundedCorners();
    if (roundedCorners.hasRadius()) {
      return (int) ViewUtils.dpToPx(roundedCorners.getRadius(), context);
    } else {
      return assetProvider.getDefaultCornerRadius();
    }
  }

  private Map<String, BindingValue> createBindingValueMap(BindingContext bindingContext) {
    Map<String, BindingValue> bindingValueMap = new HashMap<>();
    if (bindingContext.getBindingValuesCount() == 0) {
      return bindingValueMap;
    }
    for (BindingValue bindingValue : bindingContext.getBindingValuesList()) {
      bindingValueMap.put(bindingValue.getBindingId(), bindingValue);
    }
    return bindingValueMap;
  }

  /** Map that throws whenever you try to look anything up in it. */
  private static class ThrowingEmptyMap extends HashMap<String, BindingValue> {
    @Override
    public BindingValue get(/*@Nullable*/ Object key) {
      throw new IllegalArgumentException(
          "Looking up bindings not supported in this context; no BindingValues defined.");
    }
  }
}
