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

package com.google.android.libraries.feed.api.stream;

import android.os.Bundle;
import android.view.View;
import java.util.List;

/** Interface used for interacting with the Stream library in order to render a stream of cards. */
public interface Stream {
  /** Constant used to notify host that a view's position on screen is not known. */
  int POSITION_NOT_KNOWN = Integer.MIN_VALUE;

  /**
   * Called when the Stream is being created. This method should only be called once. {@code
   * savedInstanceState} should be a previous bundle returned from {@link #getSavedInstanceState()}.
   * The Stream will restore from this bundle as it starts up. This maps similarly to {@link
   * android.app.Activity#onCreate(Bundle)}.
   *
   * @param savedInstanceState state to restore to.
   * @throws IllegalStateException if method is called multiple times.
   */
  void onCreate(/*@Nullable*/ Bundle savedInstanceState);

  /**
   * Called when the Stream is visible on the screen, may be partially obscured or about to be
   * displayed. This maps similarly to {@link android.app.Activity#onStart()}.
   *
   * <p>This will cause the Stream to start pre-warming feed services.
   */
  void onShow();

  /**
   * Called when the Stream's view is completely visible and may be acted on by a user. This maps
   * similarly to {@link android.app.Activity#onResume()}.
   */
  void onActive();
  /**
   * Called when the Stream view may not be acted on by a user. Generally when the Stream is no
   * longer fully visible. Should also be called when the Activity hosting the Stream is not the
   * active activity even though Stream may be fully visible. This maps similarly to {@link
   * android.app.Activity#onPause()}.
   */
  void onInactive();

  /**
   * Called when the Stream is no longer visible on screen. This should act similarly to {@link
   * android.app.Activity#onStop()}.
   */
  void onHide();

  /**
   * Called when the Stream is destroyed. This should act similarly to {@link
   * android.app.Activity#onDestroy()}.
   */
  void onDestroy();

  /**
   * Called during {@link android.app.Activity#onSaveInstanceState(Bundle)}. The returned bundle
   * should be passed to {@link #onCreate(Bundle)} when the activity is recreated and the stream
   * needs to be recreated.
   */
  Bundle getSavedInstanceState();

  /**
   * Return the root view which holds all card stream views. The Feed library builds Views when this
   * method is called (caches as needed). This must be called after {@link #onCreate(Bundle)}.
   * Multiple calls to this method will return the same View.
   *
   * @throws IllegalStateException when called before {@link #onCreate(Bundle)}.
   */
  View getView();

  /**
   * Set headers on the stream. Headers will be added to the top of the stream in the order they are
   * in the list. The headers are not sticky and will scroll with content. Headers can be cleared by
   * passing in an empty list.
   */
  void setHeaderViews(List<View> headers);

  /**
   * Sets whether or not the Stream should show Feed content. Header views will still be shown if
   * set.
   */
  void setStreamContentVisibility(boolean visible);

  /**
   * Notifies the Stream to purge unnecessary memory. This just purges recycling pools for now. Can
   * expand out as needed.
   */
  void trim();

  /**
   * Called by the host to scroll the Stream by a certain amount. If the Stream is unable to scroll
   * the desired amount, it will scroll as much as possible.
   *
   * @param dx amount in pixels for Stream to scroll horizontally
   * @param dy amount in pixels for Stream to scroll vertically
   */
  void smoothScrollBy(int dx, int dy);

  /**
   * Returns the top position in pixels of the View at the {@code position} in the vertical
   * hierarchy. This should act similarly to {@code RecyclerView.getChildAt(position).getTop()}.
   *
   * <p>Returns {@link #POSITION_NOT_KNOWN} if position is not known. This could be returned if
   * {@code position} it not a valid position or the actual child has not been placed on screen and
   * rendered.
   */
  int getChildTopAt(int position);

  /**
   * Returns true if the child at the position is visible on screen. The view could be partially
   * visible and this would still return true.
   */
  boolean isChildAtPositionVisible(int position);

  void addScrollListener(ScrollListener listener);

  void removeScrollListener(ScrollListener listener);

  void addOnContentChangedListener(ContentChangedListener listener);

  void removeOnContentChangedListener(ContentChangedListener listener);

  /** Allow the container to trigger a refresh of the stream. */
  void triggerRefresh();
}
