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
import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.support.annotation.VisibleForTesting;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.piet.DebugLogger.MessageType;
import com.google.android.libraries.feed.piet.TemplateInstanceAdapter.TemplateAdapterModel;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.search.now.ui.piet.ActionsProto.VisibilityAction;
import com.google.search.now.ui.piet.ElementsProto.BindingContext;
import com.google.search.now.ui.piet.ElementsProto.Slice;
import com.google.search.now.ui.piet.PietAndroidSupport.ShardingControl;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * An adapter which manages {@link Frame} instances. Frames will contain one or more slices. This
 * class has additional public methods to support host access to the primary view of the frame
 * before the model is bound to the frame. A frame is basically a vertical LinearLayout of slice
 * Views which are created by {@link ElementAdapter}. This Adapter is not created through a Factory
 * and is managed by the host.
 */
public class FrameAdapter {

  private static final String TAG = "FrameAdapter";

  private final Set<ElementAdapter<?, ?>> childAdapters;

  private final Context context;
  private final AdapterParameters parameters;
  private final ActionHandler actionHandler;
  private final DebugBehavior debugBehavior;
  private final AssetProvider assetProvider;
  private final CustomElementProvider customElementProvider;
  private final HostBindingProvider hostBindingProvider;
  private final Set<VisibilityAction> activeActions = new HashSet<>();
  /*@Nullable*/ private LinearLayout view = null;
  /*@Nullable*/ private Frame frame = null;

  public FrameAdapter(
      Context context,
      AdapterParameters parameters,
      ActionHandler actionHandler,
      DebugBehavior debugBehavior,
      AssetProvider assetProvider,
      CustomElementProvider customElementProvider,
      HostBindingProvider hostBindingProvider) {
    this.context = context;
    this.parameters = parameters;
    this.actionHandler = actionHandler;
    this.debugBehavior = debugBehavior;
    this.assetProvider = assetProvider;
    this.customElementProvider = customElementProvider;
    this.hostBindingProvider = hostBindingProvider;
    childAdapters = new HashSet<>();
  }

  /**
   * This version of bind will support the {@link ShardingControl}. Sharding allows only part of the
   * frame to be rendered. When sharding is used, a frame is one or more LinearLayout containing a
   * subset of the full set of slices defined for the frame.
   */
  // TODO: Need to implement support for sharding
  public void bindModel(
      Frame frame,
      /*@Nullable*/ ShardingControl shardingControl,
      List<PietSharedState> pietSharedStates) {
    long startTime = System.nanoTime();
    this.frame = frame;
    FrameContext frameContext = createFrameContext(frame, pietSharedStates);
    initialBind(parameters.parentViewSupplier.get());
    activeActions.clear();
    activeActions.addAll(frame.getActions().getOnHideActionsList());
    LinearLayout frameView = checkNotNull(view);

    try {
      for (Slice slice : frame.getSlicesList()) {
        // For Slice we will create the lower level slice instead to remove the extra
        // level.
        List<ElementAdapter<?, ?>> adapters = getBoundAdaptersForSlice(slice, frameContext);
        for (ElementAdapter<?, ?> adapter : adapters) {
          childAdapters.add(adapter);
          setLayoutParamsOnChild(adapter);
          frameView.addView(adapter.getView());
        }
      }

      StyleProvider style = frameContext.getCurrentStyle();
      Drawable bg = frameContext.createBackground(style, context);
      frameView.setBackground(bg);
    } catch (RuntimeException e) {
      // TODO: Remove this once error reporting is fully implemented.
      Logger.e(TAG, e, "Catch top level exception");
      frameContext.reportError(MessageType.ERROR, "Top Level Exception was caught - see logcat");
    }
    startTime = System.nanoTime() - startTime;
    // TODO: We should be targeting < 15ms and warn at 10ms?
    //   Until we get a chance to do the performance tuning, leave this at 30ms to prevent
    //   warnings on large GridRows based frames.
    if (startTime / 1000000 > 30) {
      Logger.w(
          TAG,
          frameContext.reportError(
              MessageType.WARNING,
              String.format("Slow Bind (%s) time: %s ps", frame.getTag(), startTime / 1000)));
    }
    // If there were errors add an error slice to the frame
    if (frameContext.getDebugBehavior().getShowDebugViews()) {
      View errorView = frameContext.getDebugLogger().getReportView(MessageType.ERROR, context);
      if (errorView != null) {
        frameView.addView(errorView);
      }
      View warningView = frameContext.getDebugLogger().getReportView(MessageType.WARNING, context);
      if (warningView != null) {
        frameView.addView(warningView);
      }
    }
  }

  public void unbindModel() {
    LinearLayout view = checkNotNull(this.view);
    for (ElementAdapter<?, ?> child : childAdapters) {
      parameters.elementAdapterFactory.releaseAdapter(child);
    }
    childAdapters.clear();
    view.removeAllViews();
    frame = null;
  }

  private void setLayoutParamsOnChild(ElementAdapter<?, ?> childAdapter) {
    int width = childAdapter.getComputedWidthPx();
    width = width == ElementAdapter.DIMENSION_NOT_SET ? LayoutParams.MATCH_PARENT : width;
    int height = childAdapter.getComputedHeightPx();
    height = height == ElementAdapter.DIMENSION_NOT_SET ? LayoutParams.WRAP_CONTENT : height;

    childAdapter.setLayoutParams(new FrameLayout.LayoutParams(width, height));
  }

  /**
   * Return the LinearLayout managed by this FrameAdapter. This method can be used to gain access to
   * this view before {@code bindModel} is called.
   */
  public LinearLayout getFrameContainer() {
    initialBind(parameters.parentViewSupplier.get());
    return checkNotNull(view);
  }

  @VisibleForTesting
  FrameContext createFrameContext(Frame frame, List<PietSharedState> pietSharedStates) {
    return FrameContext.createFrameContext(
        frame,
        DEFAULT_STYLE,
        pietSharedStates,
        debugBehavior,
        new DebugLogger(),
        assetProvider,
        customElementProvider,
        hostBindingProvider,
        actionHandler);
  }

  public void triggerViewActions(View viewport) {
    Frame localFrame = frame;
    if (localFrame == null || view == null) {
      return;
    }
    ViewUtils.maybeTriggerViewActions(
        view, viewport, localFrame.getActions(), actionHandler, localFrame, activeActions);

    for (ElementAdapter<?, ?> adapter : childAdapters) {
      adapter.triggerViewActions(viewport);
    }
  }

  @VisibleForTesting
  List<ElementAdapter<?, ?>> getBoundAdaptersForSlice(Slice slice, FrameContext frameContext) {
    switch (slice.getSliceInstanceCase()) {
      case INLINE_SLICE:
        ElementListAdapter inlineSliceAdapter =
            parameters.elementAdapterFactory.createElementListAdapter(
                slice.getInlineSlice(), frameContext);
        inlineSliceAdapter.createAdapter(slice.getInlineSlice(), frameContext);
        inlineSliceAdapter.bindModel(slice.getInlineSlice(), frameContext);
        return Collections.singletonList(inlineSliceAdapter);
      case TEMPLATE_SLICE:
        List<ElementAdapter<?, ?>> returnList = new ArrayList<>();
        for (BindingContext bindingContext : slice.getTemplateSlice().getBindingContextsList()) {
          TemplateAdapterModel model =
              new TemplateAdapterModel(
                  slice.getTemplateSlice().getTemplateId(), frameContext, bindingContext);
          TemplateInstanceAdapter templateAdapter =
              parameters.elementAdapterFactory.createTemplateAdapter(
                  slice.getTemplateSlice(), frameContext);
          templateAdapter.createAdapter(model, frameContext);
          templateAdapter.bindModel(model, frameContext);
          returnList.add(templateAdapter);
        }
        return returnList;
      default:
        Logger.wtf(TAG, "Unsupported Slice type");
        return Collections.emptyList();
    }
  }

  @VisibleForTesting
  AdapterParameters getParameters() {
    return this.parameters;
  }

  @VisibleForTesting
  /*@Nullable*/
  LinearLayout getView() {
    return this.view;
  }

  private void initialBind(/*@Nullable*/ ViewGroup parent) {
    if (view != null) {
      return;
    }
    this.view = createView(parent);
  }

  private LinearLayout createView(/*@Nullable*/ ViewGroup parent) {
    LinearLayout linearLayout = new LinearLayout(context);
    linearLayout.setOrientation(LinearLayout.VERTICAL);
    ViewGroup.LayoutParams layoutParams;
    if (parent != null && parent.getLayoutParams() != null) {
      layoutParams = new LinearLayout.LayoutParams(parent.getLayoutParams());
      layoutParams.width = LayoutParams.MATCH_PARENT;
      layoutParams.height = LayoutParams.WRAP_CONTENT;
    } else {
      layoutParams = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
    }
    linearLayout.setLayoutParams(layoutParams);
    return linearLayout;
  }
}
