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
import android.graphics.Rect;
import android.support.v4.view.ViewCompat;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.View;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.search.now.ui.piet.ActionsProto.Actions;
import com.google.search.now.ui.piet.ActionsProto.VisibilityAction;
import com.google.search.now.ui.piet.PietProto.Frame;
import java.util.Set;

/** Utility class, providing useful methods to interact with Views. */
public class ViewUtils {
  private static final String TAG = "ViewUtils";

  /** Convert DP to PX */
  public static float dpToPx(float dp, Context context) {
    DisplayMetrics metrics = context.getResources().getDisplayMetrics();
    return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, metrics);
  }

  /** Attaches the onClick action from actions to the view, executed by the handler. */
  static void setOnClickActions(
      Actions actions, View view, ActionHandler handler, FrameContext frameContext) {
    if (actions.hasOnLongClickAction()) {
      view.setOnLongClickListener(
          v -> {
            handler.handleAction(
                actions.getOnLongClickAction(), frameContext.getFrame(), view, null);
            return true;
          });
    } else {
      clearOnLongClickActions(view);
    }
    if (actions.hasOnClickAction()) {
      view.setOnClickListener(
          v -> {
            handler.handleAction(actions.getOnClickAction(), frameContext.getFrame(), view, null);
          });
    } else {
      clearOnClickActions(view);
    }
  }

  static void clearOnLongClickActions(View view) {
    view.setOnLongClickListener(null);
    view.setLongClickable(false);
  }

  /** Sets clickability to false. */
  static void clearOnClickActions(View view) {
    if (view.hasOnClickListeners()) {
      view.setOnClickListener(null);
    }

    view.setClickable(false);
  }

  /**
   * Check if this view is visible, trigger actions accordingly, and update set of active actions.
   *
   * <p>Actions are added to activeActions when they trigger, and removed when the condition that
   * caused them to trigger is no longer true. (Ex. a view action will be removed when the view goes
   * off screen)
   *
   * @param view this adapter's view
   * @param viewport the visible viewport
   * @param actions this element's actions, which might be triggered
   * @param actionHandler host-provided handler to execute actions
   * @param frame the parent frame
   * @param activeActions mutable set of currently-triggered actions; this will get updated by this
   *     method as new actions are triggered and old actions are reset.
   */
  static void maybeTriggerViewActions(
      View view,
      View viewport,
      Actions actions,
      ActionHandler actionHandler,
      Frame frame,
      Set<VisibilityAction> activeActions) {
    if (actions.getOnViewActionsCount() == 0 && actions.getOnHideActionsCount() == 0) {
      return;
    }
    // For invisible views, short-cut triggering of hide/show actions.
    if (view.getVisibility() != View.VISIBLE || !ViewCompat.isAttachedToWindow(view)) {
      activeActions.removeAll(actions.getOnViewActionsList());
      for (VisibilityAction visibilityAction : actions.getOnHideActionsList()) {
        if (activeActions.add(visibilityAction)) {
          actionHandler.handleAction(visibilityAction.getAction(), frame, view, null);
        }
      }
      return;
    }

    // Figure out overlap of viewport and view, and trigger based on proportion overlap.
    Rect viewRect = getViewRectOnScreen(view);
    Rect viewportRect = getViewRectOnScreen(viewport);

    if (viewportRect.intersect(viewRect)) {
      int viewArea = viewRect.height() * viewRect.width();
      int visibleArea = viewportRect.height() * viewportRect.width();
      float proportionVisible = ((float) visibleArea) / viewArea;

      for (VisibilityAction visibilityAction : actions.getOnViewActionsList()) {
        if (proportionVisible >= visibilityAction.getProportionVisible()) {
          if (activeActions.add(visibilityAction)) {
            actionHandler.handleAction(visibilityAction.getAction(), frame, view, null);
          }
        } else {
          activeActions.remove(visibilityAction);
        }
      }

      for (VisibilityAction visibilityAction : actions.getOnHideActionsList()) {
        if (proportionVisible < visibilityAction.getProportionVisible()) {
          if (activeActions.add(visibilityAction)) {
            actionHandler.handleAction(visibilityAction.getAction(), frame, view, null);
          }
        } else {
          activeActions.remove(visibilityAction);
        }
      }
    }
  }

  private static Rect getViewRectOnScreen(View view) {
    int[] viewLocation = new int[2];
    view.getLocationOnScreen(viewLocation);

    return new Rect(
        viewLocation[0],
        viewLocation[1],
        viewLocation[0] + view.getWidth(),
        viewLocation[1] + view.getHeight());
  }

  /** Private constructor to prevent instantiation. */
  private ViewUtils() {}
}
